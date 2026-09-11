#include "camera.hpp"
#include <algorithm>

namespace Engine {

Camera::Camera(int viewport_w, int viewport_h)
    : viewport_width(viewport_w), viewport_height(viewport_h) {}

void Camera::set_viewport(int width, int height) {
  viewport_width = width;
  viewport_height = height;
}

void Camera::update(int target_x, int target_y, int map_width, int map_height) {
  // Center camera on target
  int desired_x = target_x - viewport_width / 2;
  int desired_y = target_y - viewport_height / 2;

  // Clamp to map bounds
  if (map_width <= viewport_width) {
    x = 0;
  } else {
    x = std::clamp(desired_x, 0, map_width - viewport_width);
  }

  if (map_height <= viewport_height) {
    y = 0;
  } else {
    y = std::clamp(desired_y, 0, map_height - viewport_height);
  }
}

bool Camera::to_screen(int world_x, int world_y, int &screen_x, int &screen_y) const {
  screen_x = world_x - x;
  screen_y = world_y - y;
  return in_view(world_x, world_y);
}

void Camera::to_world(int screen_x, int screen_y, int &world_x, int &world_y) const {
  world_x = screen_x + x;
  world_y = screen_y + y;
}

bool Camera::in_view(int world_x, int world_y) const {
  return world_x >= x && world_x < x + viewport_width &&
         world_y >= y && world_y < y + viewport_height;
}

} // namespace Engine
