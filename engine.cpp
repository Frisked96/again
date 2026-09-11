#include "engine.hpp"
#include <iostream>

namespace Engine {

GameEngine::GameEngine(int map_width, int map_height)
    : map(map_height, map_width), player(1, 1, '@', "Hero") {
  init();
}

void GameEngine::init() {
  map.generate();

  // Find a walkable starting position
  bool found = false;
  for (int y = 1; y < map.get_height() - 1 && !found; ++y) {
    for (int x = 1; x < map.get_width() - 1 && !found; ++x) {
      if (map.is_walkable(x, y)) {
        player.set_pos(x, y);
        found = true;
      }
    }
  }

  Terminal::clear_screen();
}

void GameEngine::run() {
  is_running = true;

  while (is_running) {
    renderer.render(map, player);
    handle_input();
  }

  Terminal::clear_screen();
  std::cout << "Exited game. Goodbye!\n";
}

void GameEngine::handle_input() {
  Key key = terminal.read_key();
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
  }
}

} // namespace Engine
