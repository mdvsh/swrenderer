#include "tgaimage.h"
#include "geometry.h"
#include "model.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace {
    constexpr int WIDTH  = 800;
    constexpr int HEIGHT = 800;

    // TGA stores BGRA, this abstracts the channel swap
    constexpr TGAColor rgb(uint8_t r, uint8_t g, uint8_t b) {
        return {{b, g, r, 255}};
    }
    
    constexpr auto WHITE  = rgb(255, 255, 255);
    constexpr auto RED    = rgb(255, 0, 0);
}

void line(Vec2i p0, Vec2i p1, TGAImage& image, TGAColor color) {
    bool steep = std::abs(p1.y - p0.y) > std::abs(p1.x - p0.x);
    
    // transpose steep lines so we always iterate over the longer axis
    if (steep) {
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
    }
    if (p0.x > p1.x) {
        std::swap(p0, p1);
    }
    
    const int dx = p1.x - p0.x;
    const int dy = std::abs(p1.y - p0.y);
    const int y_step = (p1.y > p0.y) ? 1 : -1;
    
    // error scaled by 2*dx to avoid floating point
    int error = 0;
    int y = p0.y;
    
    for (int x = p0.x; x <= p1.x; x++) {
        // un-transpose when setting pixel
        image.set(steep ? y : x, steep ? x : y, color);

        error += 2 * dy;
        // can make branchless by always doing inc/decr, just masked multiplied
        if (error > dx) {
            y += y_step;
            error -= 2 * dx;
        }
    }
}

Vec2i project(Vec3f v) {
    return {
        static_cast<int>((v.x + 1.f) * WIDTH  * 0.5f),
        static_cast<int>((v.y + 1.f) * HEIGHT * 0.5f)
    };
}

void render_wireframe(const Model& model, TGAImage& framebuffer, TGAColor color) {
    for (int i = 0; i < model.nfaces(); i++) {
        auto [v0, v1, v2] = model.face_verts(i);
        Vec2i s0 = project(v0);
        Vec2i s1 = project(v1);
        Vec2i s2 = project(v2);
        line(s0, s1, framebuffer, color);
        line(s1, s2, framebuffer, color);
        line(s2, s0, framebuffer, color);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "usage: " << argv[0] << " model.obj [model2.obj ...]\n";
        return 1;
    }
    
    TGAImage framebuffer(WIDTH, HEIGHT, TGAImage::RGB);
    
    for (int i = 1; i < argc; i++) {
        Model model(argv[i]);
        render_wireframe(model, framebuffer, WHITE);
    }
    
    framebuffer.write_tga_file("framebuffer.tga");
    return 0;
}