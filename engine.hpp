#pragma once
#include "entity.hpp"
#include "game_map.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace Engine {

class GameEngine {
private:
  GameMap::map map;
  Entity player;
  Terminal terminal;
  Renderer renderer;
  bool is_running{false};

  void handle_input();
  void init();

public:
  explicit GameEngine(int map_width = 50, int map_height = 18);
  void run();
};

} // namespace Engine
