#include "game_map.hpp"
#include "map_generator.hpp"
#include "utils.hpp"
#include <algorithm>

namespace GameMap {

Map::Map(int w, int h, Tile::ID default_tile)
    : width(w), height(h), grid(w * h, default_tile) {}

void Map::resize(int w, int h, Tile::ID default_tile) {
  width = w;
  height = h;
  grid.assign(w * h, default_tile);
}

void Map::clear(Tile::ID fill_id) {
  std::fill(grid.begin(), grid.end(), fill_id);
}

void Map::set(int x, int y, Tile::ID id) {
  if (!in_bounds(x, y)) {
    return;
  }
  int index = Utils::to_index(x, y, width);
  grid[index] = id;
}

Tile::ID Map::at(int x, int y) const {
  if (!in_bounds(x, y)) {
    return Tile::ID::Void;
  }
  int index = Utils::to_index(x, y, width);
  return grid[index];
}

bool Map::in_bounds(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool Map::is_walkable(int x, int y) const {
  if (!in_bounds(x, y)) {
    return false;
  }
  return !Tile::getData(at(x, y)).blocksMovement;
}

bool Map::blocks_sight(int x, int y) const {
  if (!in_bounds(x, y)) {
    return true;
  }
  return Tile::getData(at(x, y)).blocksSight;
}

void Map::generate() {
  MapGenerator::generate(*this);
}

} // namespace GameMap
