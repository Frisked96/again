#pragma once
#include "camera.hpp"
#include "entity.hpp"
#include "fov.hpp"
#include "game_map.hpp"

namespace Engine {

class Terminal;

class Renderer {
public:
  Renderer() = default;

  void render(const GameMap::Map &map,
              const Entity &player,
              const Camera &camera,
              const FOV &fov,
              Terminal &terminal);
};

} // namespace Engine
