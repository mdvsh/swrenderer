#pragma once

template<typename T>
struct Vec2 {
    T x{}, y{};
    
    constexpr Vec2() = default;
    constexpr Vec2(T x, T y) : x(x), y(y) {}
};

template<typename T>
struct Vec3 {
    T x{}, y{}, z{};
    
    constexpr Vec3() = default;
    constexpr Vec3(T x, T y, T z) : x(x), y(y), z(z) {}
    
    constexpr Vec2<T> xy() const { return {x, y}; }
};

using Vec2f = Vec2<float>;
using Vec2i = Vec2<int>;
using Vec3f = Vec3<float>;