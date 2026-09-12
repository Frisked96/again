// tile.hpp
#pragma once
#include <cstdint>
#include <string_view>

namespace Tile {

enum class ID : std::uint8_t {
  Void = 0,

  // Water Bodies
  DeepWater,
  CoastalWater,
  River,
  RiverFord,
  MarshWater,
  Ice,
  PackIce,

  // Lowlands & Vegetation
  Grassland,
  DrySteppe,
  ForestDeciduous,
  ForestConiferous,
  DenseScrub,
  PeatBog,
  Tundra,

  // Highlands, Snow & Barren
  DesertSand,
  HardenedClay,
  RockyGround,
  Foothills,
  Cliff,
  MountainPeak,
  SnowLight,
  SnowDeep,

  // Anthropogenic / Infrastructure
  DirtRoad,
  Cobblestone,
  Farmland,
  StoneWall,
  WoodWall,

  Count
};

struct Data {
  std::string_view name;
  char glyph;
  bool blocksMovement;
  bool blocksSight;
  float movementCost;
  uint8_t flammability; // 0 - 100
};

// Constexpr lookup – zero runtime overhead
inline constexpr Data getData(ID id) noexcept {
  switch (id) {
  case ID::Void:
    return {"void", ' ', true, true, 999.0f, 0};

  // Water Bodies
  case ID::DeepWater:
    return {"deep water", '~', true, false, 999.0f, 0};
  case ID::CoastalWater:
    return {"coastal shallows", '~', false, false, 2.5f, 0};
  case ID::River:
    return {"rushing river", '=', true, false, 999.0f, 0};
  case ID::RiverFord:
    return {"river ford", '=', false, false, 1.8f, 0};
  case ID::MarshWater:
    return {"stagnant bogwater", '~', false, false, 2.2f, 0};
  case ID::Ice:
    return {"lake ice", '-', false, false, 1.2f, 0};
  case ID::PackIce:
    return {"jagged pack ice", '%', false, false, 1.8f, 0};

  // Lowlands & Vegetation
  case ID::Grassland:
    return {"grassland meadow", '.', false, false, 1.0f, 20};
  case ID::DrySteppe:
    return {"arid steppe", ',', false, false, 1.0f, 30};
  case ID::ForestDeciduous:
    return {"broadleaf forest", 'T', false, true, 1.5f, 70};
  case ID::ForestConiferous:
    return {"pine taiga", 'Y', false, true, 1.6f, 80};
  case ID::DenseScrub:
    return {"dense scrub", '*', false, true, 1.8f, 50};
  case ID::PeatBog:
    return {"peat mire", ';', false, false, 2.0f, 10};
  case ID::Tundra:
    return {"mossy tundra", '_', false, false, 1.2f, 10};

  // Highlands, Snow & Barren
  case ID::DesertSand:
    return {"desert sand", '~', false, false, 1.5f, 0};
  case ID::HardenedClay:
    return {"baked clay", '.', false, false, 1.0f, 0};
  case ID::RockyGround:
    return {"stony scree", ':', false, false, 1.4f, 0};
  case ID::Foothills:
    return {"foothills", 'n', false, false, 1.5f, 0};
  case ID::Cliff:
    return {"sheer cliff", '^', true, true, 999.0f, 0};
  case ID::MountainPeak:
    return {"mountain summit", '^', true, true, 999.0f, 0};
  case ID::SnowLight:
    return {"powder snow", '.', false, false, 1.3f, 0};
  case ID::SnowDeep:
    return {"deep snowdrift", '*', false, false, 2.4f, 0};

  // Anthropogenic / Infrastructure
  case ID::DirtRoad:
    return {"cart path", '=', false, false, 0.8f, 0};
  case ID::Cobblestone:
    return {"paved road", '#', false, false, 0.7f, 0};
  case ID::Farmland:
    return {"furrowed field", '"', false, false, 1.1f, 25};
  case ID::StoneWall:
    return {"stone wall", '#', true, true, 999.0f, 0};
  case ID::WoodWall:
    return {"timber palisade", '#', true, true, 999.0f, 90};

  default:
    return {"unknown", '?', true, true, 999.0f, 0};
  }
}

} // namespace Tile
