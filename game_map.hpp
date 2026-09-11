#pragma once
#include "tile.hpp"
#include <vector>

namespace GameMap {

class Map {
private:
  int width{0};
  int height{0};
  std::vector<Tile::ID> grid;

public:
  Map(int w, int h, Tile::ID default_tile = Tile::ID::Floor);

  void resize(int w, int h, Tile::ID default_tile = Tile::ID::Floor);
  void clear(Tile::ID fill_id = Tile::ID::Floor);

  void set(int x, int y, Tile::ID id);
  Tile::ID at(int x, int y) const;

  bool in_bounds(int x, int y) const;
  bool is_walkable(int x, int y) const;
  bool blocks_sight(int x, int y) const;

  int get_width() const { return width; }
  int get_height() const { return height; }

  // Convenience generator delegation
  void generate();
};

// Backwards-compatible alias
using map = Map;

} // namespace GameMap
