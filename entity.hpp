#pragma once
#include <string>
#include <utility>

namespace Engine {

struct Entity {
  int x{0};
  int y{0};
  int z{0};
  char glyph{'@'};
  std::string name{"Player"};

  Entity() = default;
  Entity(int start_x, int start_y, int start_z, char g = '@', std::string n = "Player")
      : x(start_x), y(start_y), z(start_z), glyph(g), name(std::move(n)) {}
  Entity(int start_x, int start_y, char g = '@', std::string n = "Player")
      : x(start_x), y(start_y), z(0), glyph(g), name(std::move(n)) {}

  void move(int dx, int dy) {
    x += dx;
    y += dy;
  }

  void set_pos(int new_x, int new_y, int new_z = 0) {
    x = new_x;
    y = new_y;
    z = new_z;
  }

  void set_level(int new_z) {
    z = new_z;
  }
};

} // namespace Engine
