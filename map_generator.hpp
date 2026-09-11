#pragma once
#include "game_map.hpp"

namespace GameMap {

struct SpawnPoint {
  int x{1};
  int y{1};
};

class MapGenerator {
public:
  // Populates the map with terrain, rooms, doors, and features.
  // Returns a guaranteed walkable player spawn position.
  static SpawnPoint generate(Map &map);
};

} // namespace GameMap
