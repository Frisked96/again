#include "map_generator.hpp"

namespace GameMap {

SpawnPoint MapGenerator::generate(Map &map) {
  int width = map.get_width();
  int height = map.get_height();

  // 1. Fill everything with floor first
  map.clear(Tile::ID::Floor);

  // 2. Outer boundary walls
  for (int x = 0; x < width; ++x) {
    map.set(x, 0, Tile::ID::Wall);
    map.set(x, height - 1, Tile::ID::Wall);
  }
  for (int y = 0; y < height; ++y) {
    map.set(0, y, Tile::ID::Wall);
    map.set(width - 1, y, Tile::ID::Wall);
  }

  SpawnPoint spawn{1, 1};
  bool spawn_set = false;

  // 3. First interior room (Stone Keep)
  if (width >= 20 && height >= 12) {
    int rx = 5;
    int ry = 3;
    int rw = 12;
    int rh = 7;

    for (int x = rx; x < rx + rw; ++x) {
      map.set(x, ry, Tile::ID::Wall);
      map.set(x, ry + rh - 1, Tile::ID::Wall);
    }
    for (int y = ry; y < ry + rh; ++y) {
      map.set(rx, y, Tile::ID::Wall);
      map.set(rx + rw - 1, y, Tile::ID::Wall);
    }

    // Doorway into the room
    map.set(rx + rw / 2, ry + rh - 1, Tile::ID::Door);

    // Player spawns inside the room
    spawn = {rx + rw / 2, ry + rh / 2};
    spawn_set = true;
  }

  // 4. Second interior room (Eastern Sanctuary) if map is spacious
  if (width >= 60 && height >= 22) {
    int rx = width - 22;
    int ry = 6;
    int rw = 15;
    int rh = 9;

    for (int x = rx; x < rx + rw; ++x) {
      map.set(x, ry, Tile::ID::Wall);
      map.set(x, ry + rh - 1, Tile::ID::Wall);
    }
    for (int y = ry; y < ry + rh; ++y) {
      map.set(rx, y, Tile::ID::Wall);
      map.set(rx + rw - 1, y, Tile::ID::Wall);
    }

    // Doorway on west wall
    map.set(rx, ry + rh / 2, Tile::ID::Door);
  }

  // 5. Water pool
  if (width >= 35 && height >= 15) {
    int wx = 24;
    int wy = 5;
    for (int dy = 0; dy < 4; ++dy) {
      for (int dx = 0; dx < 6; ++dx) {
        map.set(wx + dx, wy + dy, Tile::ID::Water);
      }
    }
  }

  // 6. Stone pillars
  if (width >= 30 && height >= 15) {
    for (int py = 12; py < height - 3; py += 3) {
      for (int px = 8; px < width - 8; px += 8) {
        // Do not overwrite water or rooms
        if (map.at(px, py) == Tile::ID::Floor) {
          map.set(px, py, Tile::ID::Wall);
        }
      }
    }
  }

  // Fallback spawn point search if room was too small to place
  if (!spawn_set || !map.is_walkable(spawn.x, spawn.y)) {
    for (int y = 1; y < height - 1; ++y) {
      for (int x = 1; x < width - 1; ++x) {
        if (map.is_walkable(x, y)) {
          return {x, y};
        }
      }
    }
  }

  return spawn;
}

} // namespace GameMap
