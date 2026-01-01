#pragma once
#include "geometry.h"
#include <vector>
#include <array>
#include <string>

class Model {
public:
    explicit Model(const std::string& path);

    int nfaces() const { return static_cast<int>(faces_.size()); } 

    std::array<Vec3f, 3> face_verts(int i) const {
        auto [a, b, c] = faces_[i];
        return {verts_[a], verts_[b], verts_[c]};
    }

private:
    std::vector<Vec3f> verts_;
    std::vector<std::array<int, 3>> faces_;
};
