#pragma once
#include "camera.hpp"
#include "entity.hpp"
#include "game_map.hpp"
#include "vision.hpp"
#include <string>
#include <string_view>

namespace Engine {

class Renderer {
public:
  Renderer() = default;

  std::string render(const GameMap::Map &map,
                     const Entity &player,
                     const Camera &camera,
                     const Vision::FOV &fov,
                     std::string_view status_message = "");
};

} // namespace Engine
