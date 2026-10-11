#pragma once
#include "action.hpp"
#include "camera.hpp"
#include "entity.hpp"
#include "game_map.hpp"
#include "input.hpp"
#include "message_log.hpp"
#include "renderer.hpp"
#include "terminal.hpp"
#include "vision.hpp"

namespace Engine {

class GameEngine {
private:
  GameMap::Map map;
  Entity player;
  Camera camera;
  Vision::FOV fov;
  Terminal &terminal;
  Renderer renderer;
  MessageLog message_log;
  bool is_running{false};
  uint64_t turn_count{0};

  void handle_input();
  void tick();
  void init();

public:
  explicit GameEngine(Terminal &term, int map_width = 10000, int map_height = 10000,
                      int viewport_width = 50, int viewport_height = 18,
                      int fov_radius = 8);
  void run();
};

} // namespace Engine
