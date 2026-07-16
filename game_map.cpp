#include "game_map.hpp"
#include "utils.hpp"
#include <cassert>
namespace GameMap {
void map::bound_check(int x, int y) const {
  assert(x >= 0 && x < width && "x coordinate out of bounds!");
  assert(y >= 0 && y < height && "y coordinate out of bounds!");
}

map::map(int h, int w) : height(h), width(w), grid(h * w) {}

void map::set(int x, int y, Tile::ID id) {
  bound_check(x, y);
  int index = Utils::to_index(x, y, width);
  grid[index] = id;
}

Tile::ID map::at(int x, int y) const {
  bound_check(x, y);
  int index = Utils::to_index(x, y, width);
  return grid[index];
}

int map::get_width() const { return width; }

int map::get_height() const { return height; }
} // namespace GameMap