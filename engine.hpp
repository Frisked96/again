#pragma once
#include "camera.hpp"
#include "entity.hpp"
#include "fov.hpp"
#include "game_map.hpp"
#include "renderer.hpp"
#include "terminal.hpp"

namespace Engine {

class GameEngine {
private:
  GameMap::map map;
  Entity player;
  Camera camera;
  FOV fov;
  Terminal terminal;
  Renderer renderer;
  bool is_running{false};

  void handle_input();
  void init();

public:
  explicit GameEngine(int map_width = 80, int map_height = 35,
                      int viewport_width = 50, int viewport_height = 18,
                      int fov_radius = 8);
  void run();
};

} // namespace Engine
