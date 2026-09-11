#include "game_map.hpp"
#include "utils.hpp"
#include <cassert>
namespace GameMap {
void map::bound_check(int x, int y) const {
  assert(x >= 0 && x < width && "x coordinate out of bounds!");
  assert(y >= 0 && y < height && "y coordinate out of bounds!")
      ;
}

map::map(int h, int w) : height(h), width(w), grid(h * w, Tile::ID::Floor) {}

void map::set(int x, int y, Tile::ID id) {
  bound_check(x, y);
  int index = Utils::to_index(x, y, width);
  grid[index] = id;
}

void map::generate() {
  // Fill everything with floor first
  std::fill(grid.begin(), grid.end(), Tile::ID::Floor);

  // Outer boundary walls
  for (int x = 0; x < width; ++x) {
    set(x, 0, Tile::ID::Wall);
    set(x, height - 1, Tile::ID::Wall);
  }
  for (int y = 0; y < height; ++y) {
    set(0, y, Tile::ID::Wall);
    set(width - 1, y, Tile::ID::Wall);
  }

  // Add an interior room if map is big enough
  if (width >= 20 && height >= 12) {
    int rx = 5;
    int ry = 3;
    int rw = 12;
    int rh = 7;

    for (int x = rx; x < rx + rw; ++x) {
      set(x, ry, Tile::ID::Wall);
      set(x, ry + rh - 1, Tile::ID::Wall);
    }
    for (int y = ry; y < ry + rh; ++y) {
      set(rx, y, Tile::ID::Wall);
      set(rx + rw - 1, y, Tile::ID::Wall);
    }

    // Doorway into the room
    set(rx + rw / 2, ry + rh - 1, Tile::ID::Door);
  }

  // Add a water pool if map permits
  if (width >= 35 && height >= 15) {
    int wx = 24;
    int wy = 5;
    for (int dy = 0; dy < 4; ++dy) {
      for (int dx = 0; dx < 6; ++dx) {
        set(wx + dx, wy + dy, Tile::ID::Water);
      }
    }
  }

  // Add some stone pillars
  if (width >= 30 && height >= 15) {
    for (int py = 12; py < height - 3; py += 3) {
      for (int px = 8; px < width - 8; px += 8) {
        set(px, py, Tile::ID::Wall);
      }
    }
  }
}

Tile::ID map::at(int x, int y) const {
  bound_check(x, y);
  int index = Utils::to_index(x, y, width);
  return grid[index];
}

bool map::in_bounds(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool map::is_walkable(int x, int y) const {
  if (!in_bounds(x, y)) {
    return false;
  }
  return !Tile::getData(at(x, y)).blocksMovement;
}

int map::get_width() const { return width; }

int map::get_height() const { return height; }
} // namespace GameMap
