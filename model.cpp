#include "model.h"
#include <fstream>
#include <iostream>
#include <sstream>

Model::Model(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "failed to open " << path << "\n";
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.starts_with("v ")) {
            std::istringstream iss(line.substr(2));   
            Vec3f v;
            iss >> v.x >> v.y >> v.z;
            verts_.push_back(v);
        } else if (line.starts_with("f ")) {
            std::istringstream iss(line.substr(2));
            std::array<int, 3> face;
            std::string token;
            for (int i = 0; i < 3 && (iss >> token); ++i) {
                // handle cases like "f 1", "f 1/2", "f 1/2/3", "f 1//3"
                face[i] = std::stoi(token) - 1;
            }
            faces_.push_back(face);
        }
    }
    std::cerr << path << ": " << verts_.size() << " vertices, " << faces_.size() << " faces\n";
}