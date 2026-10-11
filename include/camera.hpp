#pragma once

namespace Engine {

class Camera {
private:
  int x{0};
  int y{0};
  int viewport_width{50};
  int viewport_height{18};

public:
  Camera() = default;
  Camera(int viewport_w, int viewport_h);

  // Center camera on (target_x, target_y), clamped to map bounds
  void update(int target_x, int target_y, int map_width, int map_height);

  // Viewport dimensions
  int get_viewport_width() const { return viewport_width; }
  int get_viewport_height() const { return viewport_height; }
  void set_viewport(int width, int height);

  // World coordinates of viewport top-left
  int get_x() const { return x; }
  int get_y() const { return y; }
  void set_position(int new_x, int new_y) {
    x = new_x;
    y = new_y;
  }

  // Coordinate conversion helpers
  bool to_screen(int world_x, int world_y, int &screen_x, int &screen_y) const;
  void to_world(int screen_x, int screen_y, int &world_x, int &world_y) const;
  bool in_view(int world_x, int world_y) const;
};

} // namespace Engine
