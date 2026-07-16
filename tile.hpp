// tile.hpp
#pragma once
#include <cstdint>
#include <string_view>

namespace Tile {

enum class ID : std::uint8_t {
  Void = 0,
  Floor,
  Wall,
  Door,
  Water,
  // add new types above
  Count
};

struct Data {
  std::string_view name;
  char glyph;
  bool blocksMovement;
  bool blocksSight;
};

// Constexpr switch-based lookup – zero overhead when ID is known at compile
// time.
inline constexpr Data getData(ID id) {
  switch (id) {
  case ID::Void:
    return {"void", ' ', true, true};
  case ID::Floor:
    return {"stone floor", '.', false, false};
  case ID::Wall:
    return {"granite wall", '#', true, true};
  case ID::Door:
    return {"wooden door", '+', true, true};
  case ID::Water:
    return {"shallow water", '~', true, false};
  default:
    return {"unknown", '?', true, true};
  }
}

} // namespace Tile