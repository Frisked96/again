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

    // Bump-to-open door interaction on current level
    if (map.get_structure(target_x, target_y, player.z).id == Structure::ID::DoorClosed) {
      map.open_door(target_x, target_y, player.z);
      fov.compute(map, player.x, player.y, player.z);
      return {true, true, "You open the door."};
    }

    if (!map.is_walkable(target_x, target_y, player.z)) {
      return {false, false, "The way is impassable."};
    }

    player.move(action.dx, action.dy);
    camera.update(player.x, player.y, map.get_width(), map.get_height());
    fov.compute(map, player.x, player.y, player.z);

    return {true, true, ""};
  }

  case ActionType::Interact: {
    // 1. Check current cell for vertical stairs
    auto current_struct = map.get_structure(player.x, player.y, player.z);
    if (current_struct.id == Structure::ID::StairsDown) {
      player.z -= 1;
      camera.update(player.x, player.y, map.get_width(), map.get_height());
      fov.compute(map, player.x, player.y, player.z);
      return {true, true, std::format("You descend the stairs to level {}.", player.z)};
    } else if (current_struct.id == Structure::ID::StairsUp) {
      player.z += 1;
      camera.update(player.x, player.y, map.get_width(), map.get_height());
      fov.compute(map, player.x, player.y, player.z);
      return {true, true, std::format("You ascend the stairs to level {}.", player.z)};
    }

    // 2. Check adjacent cells for closed/open doors
    static const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    for (const auto &d : dirs) {
      int nx = player.x + d[0];
      int ny = player.y + d[1];
      if (map.in_bounds(nx, ny)) {
        auto adj_s = map.get_structure(nx, ny, player.z);
        if (adj_s.id == Structure::ID::DoorClosed) {
          map.open_door(nx, ny, player.z);
          fov.compute(map, player.x, player.y, player.z);
          return {true, true, "You open the door."};
        } else if (adj_s.id == Structure::ID::DoorOpen) {
          map.close_door(nx, ny, player.z);
          fov.compute(map, player.x, player.y, player.z);
          return {true, true, "You close the door."};
        }
      }
    }

    // 3. Check for vegetation harvesting on ground level
    if (player.z == 0 && map.has_vegetation(player.x, player.y)) {
      auto result = map.harvest_vegetation(player.x, player.y, 25);
      if (result.amount_gathered > 0) {
        std::string msg = std::format("Harvested {} {} from {}!{}",
                                      result.amount_gathered,
                                      Vegetation::resource_name(result.resource),
                                      result.plant_name,
                                      result.depleted ? " Depleted / felled into stump!" : "");
        return {true, true, msg};
      }
    }

    return {false, false, "Nothing to interact with here."};
  }

  case ActionType::Harvest: {
    if (player.z != 0 || !map.has_vegetation(player.x, player.y)) {
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
