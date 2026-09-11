#pragma once
#include "tile.hpp"
#include <vector>

namespace GameMap {
class map {
private:
  int height, width;
  std::vector<Tile::ID> grid;
  void bound_check(int x, int y) const;

public:
  map(int h, int w);
  void set(int x, int y, Tile::ID id);
  void generate();
  Tile::ID at(int x, int y) const;
  bool in_bounds(int x, int y) const;
  bool is_walkable(int x, int y) const;
  bool blocks_sight(int x, int y) const;
  int get_width() const;
  int get_height() const;
};
} // namespace GameMap
