#include "action.hpp"
#include <format>

namespace Engine {

ActionResult ActionSystem::execute(const Action &action,
                                   GameMap::Map &map,
                                   Entity &player,
                                   Camera &camera,
                                   Vision::FOV &fov) {
  switch (action.type) {
  case ActionType::Move: {
    int target_x = player.x + action.dx;
    int target_y = player.y + action.dy;

    if (!map.in_bounds(target_x, target_y)) {
      return {false, false, "The way is blocked by the world boundary."};
    }

    if (!map.is_walkable(target_x, target_y)) {
      return {false, false, "The way is impassable."};
    }

    player.move(action.dx, action.dy);
    camera.update(player.x, player.y, map.get_width(), map.get_height());
    fov.compute(map, player.x, player.y);

    return {true, true, ""};
  }

  case ActionType::Harvest: {
    if (!map.has_vegetation(player.x, player.y)) {
      return {false, false, "No harvestable flora here."};
    }

    auto result = map.harvest_vegetation(player.x, player.y, 25);
    if (result.amount_gathered > 0) {
      std::string msg = std::format("Harvested {} {} from {}!{}",
                                    result.amount_gathered,
                                    Vegetation::resource_name(result.resource),
                                    result.plant_name,
                                    result.depleted ? " Depleted / felled into stump!" : "");
      return {true, true, msg};
    } else {
      return {false, false, "Nothing more to harvest here."};
    }
  }

  case ActionType::Wait:
    return {true, true, "You wait a moment."};

  case ActionType::Quit:
  case ActionType::None:
  default:
    return {false, false, ""};
  }
}

} // namespace Engine
