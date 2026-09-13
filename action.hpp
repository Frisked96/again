#pragma once
#include "camera.hpp"
#include "entity.hpp"
#include "game_map.hpp"
#include "input.hpp"
#include "vision.hpp"
#include <string>

namespace Engine {

struct ActionResult {
  bool success{false};
  bool consumed_turn{false};
  std::string message;
};

class ActionSystem {
public:
  static ActionResult execute(const Action &action,
                              GameMap::Map &map,
                              Entity &player,
                              Camera &camera,
                              Vision::FOV &fov);
};

} // namespace Engine
