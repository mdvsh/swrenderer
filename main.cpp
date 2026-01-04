#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <iostream>

namespace {
    constexpr int WIDTH  = 800;
    constexpr int HEIGHT = 800;

    constexpr TGAColor rgb(uint8_t r, uint8_t g, uint8_t b) {
        return {{b, g, r, 255}};  // TGA is BGRA
    }
    
    [[maybe_unused]] constexpr auto WHITE = rgb(255, 255, 255);
    [[maybe_unused]] constexpr auto RED   = rgb(255, 0, 0);
}

// ---------------------------------------------------------------------------
// line rasterization - bresenham
// ---------------------------------------------------------------------------

void line(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color) {
    bool steep = std::abs(p1.y - p0.y) > std::abs(p1.x - p0.x);
    
    // transpose to always iterate the longer axis
    if (steep) {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
    }
    if (p0.x > p1.x) std::swap(p0, p1);
    
    const int dx = p1.x - p0.x;
    const int dy = std::abs(p1.y - p0.y);
    const int y_step = (p1.y > p0.y) ? 1 : -1;
    
    // error scaled by 2*dx to stay in integers
    int error = 0;
    int y = p0.y;
    
    for (int x = p0.x; x <= p1.x; x++) {
        image.set(steep ? y : x, steep ? x : y, color);
        error += 2 * dy;
        if (error > dx) {
            y += y_step;
            error -= 2 * dx;
        }
    }
}

// ---------------------------------------------------------------------------
// triangle rasterization
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// method 1: scanline (sequential, historical)
// ---------------------------------------------------------------------------
// splits at middle vertex, fills two trapezoids. awkward but instructive.

void triangle_scanline(Vec2i a, Vec2i b, Vec2i c, TGAImage& image, TGAColor color) {
    // sort vertices by y ascending
    if (a.y > b.y) std::swap(a, b);
    if (a.y > c.y) std::swap(a, c);
    if (b.y > c.y) std::swap(b, c);

    int total_height = c.y - a.y;
    if (total_height == 0) return;
    
    for (int y = a.y; y <= c.y; y++) {
        bool upper_half = (y > b.y) || (a.y == b.y);
        int segment_height = upper_half ? (c.y - b.y) : (b.y - a.y);
        if (segment_height == 0) continue;

        float alpha = static_cast<float>(y - a.y) / total_height;
        float beta = upper_half
            ? static_cast<float>(y - b.y) / segment_height
            : static_cast<float>(y - a.y) / segment_height;
        
        // x1 interpolates along a->c (full edge), x2 switches segment edge at midpt 
        int x1 = a.x + (c.x - a.x) * alpha; 
        int x2 = upper_half ? b.x + (c.x - b.x) * beta : a.x + (b.x - a.x) * beta;
        if (x1 > x2) std::swap(x1, x2);
        for (int x = x1; x <= x2; x++) {
            image.set(x, y, color);
        }
    }
}

// ---------------------------------------------------------------------------
// method 2: barycentric (bbox iteration, gpu-style)
// ---------------------------------------------------------------------------
// P = uA + vB + wC with u+v+w=1. solve via cramer's rule on the 2x2 system.

Vec3f barycentric(const Vec2i a, const Vec2i b, const Vec2i c, const Vec2i p) {
    // rewrite P = A + u(B-A) + v(C-A), solve for (u,v)
    // the "cross product trick" is cramers rule: u = det([AP,AC])/det([AB,AC])
    Vec2i ab = b - a, ac = c - a, ap = p - a;
    
    float denom = cross2d(ab, ac);  // 2x signed area of triangle
    if (std::abs(denom) < 1e-6f) return {-1, 1, 1};  // degenerate
    
    float u = cross2d(ap, ac) / denom;
    float v = cross2d(ab, ap) / denom;
    return {1 - u - v, u, v};
}

void triangle_barycentric(Vec2i v0, Vec2i v1, Vec2i v2, TGAImage& image, TGAColor color) {
    auto [min_x, max_x, min_y, max_y] = compute_bbox(v0, v1, v2, image.width(), image.height());
    for (int y = min_y; y <= max_y; y++) {
        for (int x = min_x; x <= max_x; x++) {
            Vec3f bc = barycentric(v0, v1, v2, {x, y});
            if (bc.x < 0 || bc.y < 0 || bc.z < 0) continue;
            image.set(x, y, color);
        }
    }
}

// ---------------------------------------------------------------------------
// method 3: edge functions (incremental, what hardware does)
// ---------------------------------------------------------------------------
// E(p) = (p-a) × (b-a) is a linear function (ax + by + c form).
// positive if p left of edge a->b, negative if right, zero if on edge.
// triangle interior = intersection of three half-planes = all E's non-negative.
// since E is linear, ∂E/∂x and ∂E/∂y are constant - evaluate once at bbox
// corner, then accumulate deltas. inner loop is pure integer adds.

[[nodiscard]] constexpr int edge_fn(Vec2i a, Vec2i b, Vec2i p) {
    return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
}

void triangle_edge(Vec2i v0, Vec2i v1, Vec2i v2, TGAImage& image, TGAColor color) {
    auto [min_x, max_x, min_y, max_y] = compute_bbox(v0, v1, v2, image.width(), image.height());
    
    // force CCW winding so inside = all edge functions non-negative
    if (edge_fn(v0, v1, v2) < 0) std::swap(v1, v2);
    
    // ∂E/∂x = b.y - a.y (used when stepping x)
    // ∂E/∂y = a.x - b.x (used when stepping y)
    int dy01 = v1.y - v0.y, dx01 = v0.x - v1.x;
    int dy12 = v2.y - v1.y, dx12 = v1.x - v2.x;
    int dy20 = v0.y - v2.y, dx20 = v2.x - v0.x;
    
    Vec2i p{min_x, min_y};
    int w0_row = edge_fn(v1, v2, p);
    int w1_row = edge_fn(v2, v0, p);
    int w2_row = edge_fn(v0, v1, p);
    
    for (int y = min_y; y <= max_y; y++) {
        int w0 = w0_row, w1 = w1_row, w2 = w2_row;
        for (int x = min_x; x <= max_x; x++) {
            // bit-or trick: if any weight negative, sign bit set, fails >= 0
            if ((w0 | w1 | w2) >= 0) image.set(x, y, color);
            w0 += dy12; w1 += dy20; w2 += dy01;
        }
        w0_row += dx12; w1_row += dx20; w2_row += dx01;
    }
}

void triangle(Vec2i v0, Vec2i v1, Vec2i v2, TGAImage& image, TGAColor color) {
    triangle_edge(v0, v1, v2, image, color);  // default to fast path
}

// ---------------------------------------------------------------------------
// projection (orthographic for now, perspective + matrices coming later)
// ---------------------------------------------------------------------------

Vec2i project(Vec3f v) {
    // affine map: [-1,1]^3 -> [0,W]×[0,H], drop z
    return {
        static_cast<int>((v.x + 1.f) * WIDTH  * 0.5f),
        static_cast<int>((v.y + 1.f) * HEIGHT * 0.5f)
    };
}

// ---------------------------------------------------------------------------
// rendering
// ---------------------------------------------------------------------------

void render_wireframe(const Model& model, TGAImage& fb, TGAColor color) {
    for (int i = 0; i < model.nfaces(); i++) {
        auto [v0, v1, v2] = model.face_verts(i);
        Vec2i s0 = project(v0), s1 = project(v1), s2 = project(v2);
        line(s0, s1, fb, color);
        line(s1, s2, fb, color);
        line(s2, s0, fb, color);
    }
}

void render_flat(const Model& model, TGAImage& fb, Vec3f light_dir) {
    for (int i = 0; i < model.nfaces(); i++) {
        auto [v0, v1, v2] = model.face_verts(i);
        
        // n = (v2-v0)×(v1-v0), intensity = n·L (lambertian cosine law)
        Vec3f n = normalized(cross(v2 - v0, v1 - v0));
        float intensity = dot(n, light_dir);
        
        if (intensity <= 0) continue;  // back-face cull
        
        Vec2i s0 = project(v0), s1 = project(v1), s2 = project(v2);
        uint8_t shade = static_cast<uint8_t>(std::clamp(intensity, 0.f, 1.f) * 255.f);
        triangle(s0, s1, s2, fb, rgb(shade, shade, shade));
    }
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " model.obj [model2.obj ...]\n";
        return 1;
    }
    
    TGAImage fb(WIDTH, HEIGHT, TGAImage::RGB);
    Vec3f light = normalized(Vec3f{0, 0, -1});
    
    for (int i = 1; i < argc; i++) {
        Model model(argv[i]);
        render_flat(model, fb, light);
    }
    
    fb.write_tga_file("framebuffer.tga");
    return 0;
}