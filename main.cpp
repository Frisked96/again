#include "engine.hpp"

int main() {
  // Map dimensions: 80x35, Viewport: 50x18, FOV radius: 8
  Engine::GameEngine game(80, 35, 50, 18, 8);
  game.run();
  return 0;
}
