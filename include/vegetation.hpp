// vegetation.hpp
#pragma once
#include <cstdint>
#include <string_view>

namespace Vegetation {

enum class ID : uint8_t {
  None = 0,
  DeciduousTree,   // Broadleaf oak/birch/elm
  ConiferousTree,  // Evergreen pine/spruce/fir
  DenseScrub,      // Dense woody brambles/thicket
  BerryBush,       // Wild fruiting shrub
  WildCrops,       // Wild spelt/wheat/barley
  MarshReeds,      // Wetlands rushes/cattails
  TundraLichen,    // Subpolar crustose lichen & moss
  AridBrush,       // Desert thornbrush & succulent
  TreeStump,       // Felled tree base
  DepletedBush,    // Harvested berry shrub awaiting recovery
  Count
};

enum class ResourceType : uint8_t {
  None = 0,
  Timber,       // Hardwood timber
  Softwood,     // Pine timber
  Kindling,     // Dry branches / scrub wood
  WildBerries,  // Edible fruit
  Grain,        // Wild grain
  Thatch,       // Wetland reeds / fiber
  Tinder        // Dry moss / lichen for firestarting
};

inline constexpr std::string_view resource_name(ResourceType type) noexcept {
  switch (type) {
  case ResourceType::Timber:
    return "hardwood timber";
  case ResourceType::Softwood:
    return "softwood timber";
  case ResourceType::Kindling:
    return "kindling";
  case ResourceType::WildBerries:
    return "wild berries";
  case ResourceType::Grain:
    return "wild grain";
  case ResourceType::Thatch:
    return "thatch reed";
  case ResourceType::Tinder:
    return "tinder moss";
  default:
    return "none";
  }
}

struct Data {
  std::string_view name;
  char glyph;
  bool blocksSight;
  bool blocksMovement;
  float movementCostMult;
  uint8_t flammability; // 0 - 100
  uint8_t maxYield;
  ResourceType resourceType;
  // Climatic tolerance thresholds
  float minTemp;     // Minimum viable temperature in Celsius
  float maxTemp;     // Maximum viable temperature in Celsius
  float minMoisture; // Minimum moisture 0.0 - 1.0
  float maxMoisture; // Maximum moisture 0.0 - 1.0
};

// Packed cell representation (2 bytes per cell: 200 MB for 100M cells)
struct Cell {
  ID id{ID::None};
  uint8_t resource_amount{0}; // Remaining harvestable yield (0 - 100%)
};

inline constexpr Data getData(ID id) noexcept {
  switch (id) {
  case ID::None:
    return {"none", ' ', false, false, 1.0f, 0, 0, ResourceType::None, -100.0f, 100.0f, 0.0f, 1.0f};

  case ID::DeciduousTree:
    return {"broadleaf oak", 'T', true, false, 1.4f, 70, 100, ResourceType::Timber,
            5.0f, 32.0f, 0.40f, 0.95f};

  case ID::ConiferousTree:
    return {"pine taiga", 'Y', true, false, 1.5f, 80, 100, ResourceType::Softwood,
            -25.0f, 18.0f, 0.30f, 0.90f};

  case ID::DenseScrub:
    return {"dense scrub", '*', true, false, 1.6f, 60, 60, ResourceType::Kindling,
            0.0f, 35.0f, 0.35f, 0.90f};

  case ID::BerryBush:
    return {"berry shrub", '%', false, false, 1.2f, 40, 30, ResourceType::WildBerries,
            4.0f, 28.0f, 0.45f, 0.85f};

  case ID::WildCrops:
    return {"wild grain", '"', false, false, 1.1f, 30, 40, ResourceType::Grain,
            8.0f, 30.0f, 0.35f, 0.80f};

  case ID::MarshReeds:
    return {"wetland reeds", ';', false, false, 1.5f, 20, 50, ResourceType::Thatch,
            0.0f, 25.0f, 0.70f, 1.0f};

  case ID::TundraLichen:
    return {"tundra lichen", '_', false, false, 1.05f, 15, 20, ResourceType::Tinder,
            -35.0f, 6.0f, 0.10f, 0.60f};

  case ID::AridBrush:
    return {"desert scrub", 'x', false, false, 1.15f, 50, 25, ResourceType::Kindling,
            15.0f, 48.0f, 0.05f, 0.35f};

  case ID::TreeStump:
    return {"tree stump", 'o', false, false, 1.1f, 40, 20, ResourceType::Kindling,
            -100.0f, 100.0f, 0.0f, 1.0f};

  case ID::DepletedBush:
    return {"stripped shrub", '.', false, false, 1.05f, 30, 0, ResourceType::None,
            -100.0f, 100.0f, 0.0f, 1.0f};

  default:
    return {"unknown flora", '?', false, false, 1.0f, 0, 0, ResourceType::None, -100.0f, 100.0f, 0.0f, 1.0f};
  }
}

inline constexpr bool can_grow(ID id, float temp_celsius, float moisture) noexcept {
  auto d = getData(id);
  return (temp_celsius >= d.minTemp && temp_celsius <= d.maxTemp &&
          moisture >= d.minMoisture && moisture <= d.maxMoisture);
}

} // namespace Vegetation
