#pragma once
#include <cmath>
#include <tuple>
#include <algorithm>

template<typename T>
struct Vec2 {
    T x{}, y{};
    
    constexpr Vec2() = default;
    constexpr Vec2(T x, T y) : x(x), y(y) {}
    
    constexpr Vec2 operator+(Vec2 v) const { return {x + v.x, y + v.y}; }
    constexpr Vec2 operator-(Vec2 v) const { return {x - v.x, y - v.y}; }
    constexpr Vec2 operator*(T s) const { return {x * s, y * s}; }
};

template<typename T>
struct Vec3 {
    T x{}, y{}, z{};
    
    constexpr Vec3() = default;
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}
    
    constexpr Vec3 operator+(Vec3 v) const { return {x + v.x, y + v.y, z + v.z}; }
    constexpr Vec3 operator-(Vec3 v) const { return {x - v.x, y - v.y, z - v.z}; }
    constexpr Vec3 operator*(T s) const { return {x * s, y * s, z * s}; }
    
    constexpr Vec2<T> xy() const { return {x, y}; }
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec3f = Vec3<float>;

template<typename T>
constexpr T dot(Vec3<T> a, Vec3<T> b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

template<typename T>
constexpr Vec3<T> cross(Vec3<T> a, Vec3<T> b) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

// 2D "cross product" is the z-component of the 3D cross with z=0
// geometrically: signed area of parallelogram, det([a|b])
template<typename T>
constexpr T cross2d(Vec2<T> a, Vec2<T> b) {
    return a.x * b.y - a.y * b.x;
}

template<typename T>
T norm(Vec3<T> v) {
    return std::sqrt(dot(v, v));
}

template<typename T>
Vec3<T> normalized(Vec3<T> v) {
    return v * (T{1} / norm(v));
}

inline auto compute_bbox(Vec2i v0, Vec2i v1, Vec2i v2, int w, int h) {
    return std::tuple{
        std::max(0, std::min({v0.x, v1.x, v2.x})),
        std::min(w - 1, std::max({v0.x, v1.x, v2.x})),
        std::max(0, std::min({v0.y, v1.y, v2.y})),
        std::min(h - 1, std::max({v0.y, v1.y, v2.y}))
    };
}
