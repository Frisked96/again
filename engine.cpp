#include "engine.hpp"
#include "map_generator.hpp"

namespace Engine {

GameEngine::GameEngine(int map_width, int map_height,
                       int viewport_width, int viewport_height,
                       int fov_radius)
    : map(map_width, map_height),
      player(1, 1, '@', "Hero"),
      camera(viewport_width, viewport_height),
      fov(map_width, map_height, fov_radius) {
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
    renderer.render(map, player, camera, fov, terminal);
    handle_input();
  }

  Terminal::clear_screen();
  terminal.write("Exited game. Goodbye!\n");
}

void GameEngine::tick() {
  ++turn_count;
  // Advance simulation (monsters, factions, environment) per Hybrid Time pillar
}

void GameEngine::handle_input() {
  // Hybrid time: wait up to 1.5s for player action before ticking the world
  Key key = terminal.read_key(1500);
  int dx = 0;
  int dy = 0;

  switch (key) {
  case Key::Up:
    dy = -1;
    break;
  case Key::Down:
    dy = 1;
    break;
  case Key::Left:
    dx = -1;
    break;
  case Key::Right:
    dx = 1;
    break;
  case Key::Timeout:
    // Hybrid time heartbeat
    tick();
    return;
  case Key::Quit:
    is_running = false;
    return;
  default:
    return;
  }

  int target_x = player.x + dx;
  int target_y = player.y + dy;

  if (map.is_walkable(target_x, target_y)) {
    player.move(dx, dy);
    camera.update(player.x, player.y, map.get_width(), map.get_height());
    fov.compute(map, player.x, player.y);
    tick();
  }
}

} // namespace Engine
