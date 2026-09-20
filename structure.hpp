// structure.hpp
#pragma once
#include <cstdint>
#include <string_view>

namespace Structure {

enum class ID : uint8_t {
  None = 0,

  // Walls & Fortifications
  WoodWall,
  StoneWall,
  Palisade,

  // Floors & Pavements
  WoodFloor,
  StoneFloor,

  // Portals & Openings
  DoorClosed,
  DoorOpen,
  Window,
  IronBars,

  // Vertical Transitions
  StairsDown, // '>'
  StairsUp,   // '<'
  Ladder,     // 'H'

  // Furniture & Fixtures
  Anvil,
  Counter,

  // Debris & Ruins (Destruction states)
  Rubble,
  WoodScraps,

  Count
};

struct Cell {
  ID id{ID::None};
  uint8_t durability{0}; // 0 - 100%
};

struct Data {
  std::string_view name;
  char glyph;
  bool blocksMovement;
  bool blocksSight;
  float movementCostMult;
  uint8_t maxDurability;
  uint8_t flammability;
  bool isLoadBearing;
  ID debrisType;
};

inline constexpr Data getData(ID id) noexcept {
  switch (id) {
  case ID::None:
    return {"none", ' ', false, false, 1.0f, 0, 0, false, ID::None};

  // Walls & Fortifications
  case ID::WoodWall:
    return {"wooden wall", '#', true, true, 999.0f, 100, 70, true, ID::WoodScraps};
  case ID::StoneWall:
    return {"stone wall", '#', true, true, 999.0f, 100, 0, true, ID::Rubble};
  case ID::Palisade:
    return {"wooden palisade", '#', true, true, 999.0f, 80, 80, true, ID::WoodScraps};

  // Floors & Pavements
  case ID::WoodFloor:
    return {"wooden plank floor", '.', false, false, 1.0f, 60, 50, false, ID::WoodScraps};
  case ID::StoneFloor:
    return {"flagstone floor", '.', false, false, 1.0f, 100, 0, false, ID::Rubble};

  // Portals
  case ID::DoorClosed:
    return {"closed wooden door", '+', true, true, 999.0f, 80, 60, true, ID::WoodScraps};
  case ID::DoorOpen:
    return {"open wooden door", '/', false, false, 1.0f, 80, 60, false, ID::WoodScraps};
  case ID::Window:
    return {"glass window", '"', true, false, 999.0f, 30, 0, false, ID::Rubble};
  case ID::IronBars:
    return {"iron portcullis", '#', true, false, 999.0f, 100, 0, true, ID::Rubble};

  // Vertical Transitions
  case ID::StairsDown:
    return {"staircase descending", '>', false, false, 1.0f, 100, 0, false, ID::Rubble};
  case ID::StairsUp:
    return {"staircase ascending", '<', false, false, 1.0f, 100, 0, false, ID::Rubble};
  case ID::Ladder:
    return {"sturdy ladder", 'H', false, false, 1.2f, 50, 40, false, ID::WoodScraps};

  // Furniture & Fixtures
  case ID::Anvil:
    return {"blacksmith anvil", '&', true, false, 999.0f, 100, 0, false, ID::None};
  case ID::Counter:
    return {"wooden counter", '=', true, false, 999.0f, 60, 50, false, ID::WoodScraps};

  // Debris & Ruins
  case ID::Rubble:
    return {"pile of stone rubble", '%', false, false, 2.0f, 40, 0, false, ID::None};
  case ID::WoodScraps:
    return {"splintered wood scraps", ';', false, false, 1.5f, 30, 90, false, ID::None};

  case ID::Count:
  default:
    return {"unknown", '?', false, false, 1.0f, 0, 0, false, ID::None};
  }
}

inline constexpr bool is_floor(ID id) noexcept {
  return id == ID::WoodFloor || id == ID::StoneFloor || id == ID::StairsDown ||
         id == ID::StairsUp || id == ID::Ladder;
}

inline constexpr bool is_portal(ID id) noexcept {
  return id == ID::DoorClosed || id == ID::DoorOpen;
}

} // namespace Structure
