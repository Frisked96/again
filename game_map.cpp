#include "game_map.hpp"
#include <iostream>

void game_map::generate() {
    for (int id = 0; id < depth; ++id) {
            for (int iy = 0; iy < height; ++iy) {
                for (int ix = 0; ix < width; ++ix) {
                    map.push_back(voxel::Wall);
                }
            }
        }
}

char game_map::get_glyph(int x, int y, int z) const {
    if (x < 0 || x >= width || y < 0 || y >= height || z < 0 || z >= depth) {
        return ' ';
    }
    size_t index = static_cast<size_t>(z) * (static_cast<size_t>(height) * static_cast<size_t>(width))
                   + static_cast<size_t>(y) * static_cast<size_t>(width)
                   + static_cast<size_t>(x);
    return map[index].glyph;
}

void game_map::print(int h, int w, int d) {
    int ph = (h > 0) ? h : height;
    int pw = (w > 0) ? w : width;
    int pd = (d > 0) ? d : depth;

    for (int z = 0; z < pd; ++z) {
        std::cout << "Layer " << z << ":\n";
        for (int y = 0; y < ph; ++y) {
            for (int x = 0; x < pw; ++x) {
                std::cout << get_glyph(x, y, z);
            }
            std::cout << '\n';
        }
        std::cout << '\n';
    }
}