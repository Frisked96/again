#pragma once
#include "entity.hpp"
#include "game_map.hpp"

namespace Engine {

class Renderer {
public:
  Renderer() = default;

  void render(const GameMap::map &map, const Entity &player);
};

} // namespace Engine
