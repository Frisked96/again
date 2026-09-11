#pragma once
#include "camera.hpp"
#include "entity.hpp"
#include "fov.hpp"
#include "game_map.hpp"

namespace Engine {

class Renderer {
public:
  Renderer() = default;

  void render(const GameMap::map &map,
              const Entity &player,
              const Camera &camera,
              const FOV &fov);
};

} // namespace Engine
