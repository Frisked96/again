#include "tile.hpp"
#include <vector>

class game_map {
    private:
    int height;
    int width;
    int depth;
    std::vector<voxel::Block> map;

    public:
    game_map(int h, int w, int d): height(h), width(w), depth(d) {}
    void generate();
    void print(int h, int w, int d);
    char get_glyph(int x, int y, int z) const;
    ~game_map();
};