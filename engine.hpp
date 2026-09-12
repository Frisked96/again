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
  GameMap::Map map;
  Entity player;
  Camera camera;
  FOV fov;
  Terminal terminal;
  Renderer renderer;
  bool is_running{false};
  uint64_t turn_count{0};

  void handle_input();
  void tick();
  void init();

public:
  explicit GameEngine(int map_width = 10000, int map_height = 10000,
                      int viewport_width = 50, int viewport_height = 18,
                      int fov_radius = 8);
  void run();
};

} // namespace Engine
