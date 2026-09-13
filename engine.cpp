#include "engine.hpp"
#include "map_generator.hpp"

namespace Engine {

GameEngine::GameEngine(int map_width, int map_height,
                       int viewport_width, int viewport_height,
                       int fov_radius)
    : map(map_width, map_height),
      player(1, 1, '@', "Hero"),
      camera(viewport_width, viewport_height),
      fov(map_width, map_height, fov_radius),
      message_log("Explore the realm.") {
  init();
}

void GameEngine::init() {
  // Generate map and place player at safe spawn point
  auto spawn = GameMap::MapGenerator::generate(map);
  player.set_pos(spawn.x, spawn.y);

  // Initial camera tracking and FOV computation
  camera.update(player.x, player.y, map.get_width(), map.get_height());
  fov.compute(map, player.x, player.y);

  Terminal::clear_screen();
}

void GameEngine::run() {
  is_running = true;

  while (is_running) {
    // Decoupled presentation pipeline:
    // 1. Renderer constructs frame buffer
    // 2. Terminal presents frame buffer
    std::string frame = renderer.render(map, player, camera, fov, message_log.get());
    terminal.present(frame);

    handle_input();
  }

  Terminal::clear_screen();
  terminal.write("Exited game. Goodbye!\n");
}

void GameEngine::tick() {
  ++turn_count;
  // Advance world simulation (wildlife, weather, factions) per Hybrid Time pillar
}

void GameEngine::handle_input() {
  // Hybrid time: wait up to 1.5s for player action before ticking the world
  RawKey raw_key = terminal.read_key(1500);
  Action action = InputHandler::map_key_to_action(raw_key);

  if (action.type == ActionType::Quit) {
    is_running = false;
    return;
  }

  // Execute gameplay action
  ActionResult result = ActionSystem::execute(action, map, player, camera, fov);

  if (!result.message.empty()) {
    message_log.set(result.message);
  } else if (result.consumed_turn) {
    message_log.clear();
  }

  if (result.consumed_turn) {
    tick();
  }
}

} // namespace Engine
