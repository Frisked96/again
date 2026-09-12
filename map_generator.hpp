#pragma once
#include "game_map.hpp"
#include <cstdint>

namespace GameMap {

struct SpawnPoint {
  int x{1};
  int y{1};
};

class MapGenerator {
public:
  // Populates the 10,000 x 10,000 world map with realistic medieval geography:
  // - Continental macro-biomes (Whittaker climate model: Elevation, Moisture, Temperature)
  // - Static, predetermined regional climates
  // - Natural terrain distribution (rivers, forests, dunes, alpine crags, farmland)
  // Returns a safe, walkable player spawn point.
  static SpawnPoint generate(Map &map, uint32_t seed = 42);
};

} // namespace GameMap
