#pragma once
#include <string>
#include <utility>

namespace Engine {

struct Entity {
  int x{0};
  int y{0};
  char glyph{'@'};
  std::string name{"Player"};

  Entity() = default;
  Entity(int start_x, int start_y, char g = '@', std::string n = "Player")
      : x(start_x), y(start_y), glyph(g), name(std::move(n)) {}

  void move(int dx, int dy) {
    x += dx;
    y += dy;
  }

  void set_pos(int new_x, int new_y) {
    x = new_x;
    y = new_y;
  }
};

} // namespace Engine
