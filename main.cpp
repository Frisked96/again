#include "engine.hpp"

int main() {
  // Map dimensions: 10000x10000, Viewport: 50x18, FOV radius: 8
  Engine::GameEngine game(10000, 10000, 50, 18, 8);
  game.run();
  return 0;
}
