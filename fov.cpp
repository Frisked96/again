#include "fov.hpp"
#include "game_map.hpp"
#include "utils.hpp"
#include <algorithm>

namespace Engine {

FOV::FOV(int w, int h, int r)
    : width(w), height(h), radius(r),
      visible_grid(w * h, false), explored_grid(w * h, false) {}

void FOV::resize(int w, int h) {
  width = w;
  height = h;
  visible_grid.assign(w * h, false);
  explored_grid.assign(w * h, false);
}

bool FOV::in_bounds(int x, int y) const {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool FOV::is_visible(int x, int y) const {
  if (!in_bounds(x, y)) {
    return false;
  }
  return visible_grid[Utils::to_index(x, y, width)];
}

bool FOV::is_explored(int x, int y) const {
  if (!in_bounds(x, y)) {
    return false;
  }
  return explored_grid[Utils::to_index(x, y, width)];
}

void FOV::set_visible(int x, int y, bool visible) {
  if (in_bounds(x, y)) {
    visible_grid[Utils::to_index(x, y, width)] = visible;
  }
}

void FOV::set_explored(int x, int y, bool explored) {
  if (in_bounds(x, y)) {
    explored_grid[Utils::to_index(x, y, width)] = explored;
  }
}

void FOV::reveal_all() {
  std::fill(visible_grid.begin(), visible_grid.end(), true);
  std::fill(explored_grid.begin(), explored_grid.end(), true);
}

void FOV::reset() {
  std::fill(visible_grid.begin(), visible_grid.end(), false);
  std::fill(explored_grid.begin(), explored_grid.end(), false);
}

void FOV::scan(const GameMap::map &map, int cx, int cy, int row,
               float start_slope, float end_slope, int current_radius,
               int xx, int xy, int yx, int yy) {
  if (start_slope < end_slope) {
    return;
  }

  float next_start_slope = start_slope;

  for (int j = row; j <= current_radius; ++j) {
    int dx = -j - 1;
    int dy = -j;
    bool blocked = false;

    while (dx <= 0) {
      dx++;
      int X = cx + dx * xx + dy * xy;
      int Y = cy + dx * yx + dy * yy;

      float l_slope = (dx - 0.5f) / (dy + 0.5f);
      float r_slope = (dx + 0.5f) / (dy - 0.5f);

      if (start_slope < r_slope) {
        continue;
      }
      if (end_slope > l_slope) {
        break;
      }

      // Check distance (circular sight radius)
      if (dx * dx + dy * dy <= current_radius * current_radius) {
        if (in_bounds(X, Y)) {
          int index = Utils::to_index(X, Y, width);
          visible_grid[index] = true;
          explored_grid[index] = true;
        }
      }

      bool is_blocking = !map.in_bounds(X, Y) || map.blocks_sight(X, Y);

      if (blocked) {
        if (is_blocking) {
          next_start_slope = r_slope;
          continue;
        } else {
          blocked = false;
          start_slope = next_start_slope;
        }
      } else {
        if (is_blocking && j < current_radius) {
          blocked = true;
          scan(map, cx, cy, j + 1, start_slope, l_slope, current_radius,
               xx, xy, yx, yy);
          next_start_slope = r_slope;
        }
      }
    }

    if (blocked) {
      break;
    }
  }
}

void FOV::compute(const GameMap::map &map, int player_x, int player_y, int r) {
  if (r > 0) {
    radius = r;
  }

  // Clear previous turn's visibility
  std::fill(visible_grid.begin(), visible_grid.end(), false);

  // Player's own tile is always visible and explored
  if (in_bounds(player_x, player_y)) {
    int idx = Utils::to_index(player_x, player_y, width);
    visible_grid[idx] = true;
    explored_grid[idx] = true;
  }

  // 8 octant multipliers
  static const int multipliers[4][8] = {
      { 1,  0,  0, -1, -1,  0,  0,  1 },
      { 0,  1, -1,  0,  0, -1,  1,  0 },
      { 0,  1,  1,  0,  0, -1, -1,  0 },
      { 1,  0,  0,  1, -1,  0,  0, -1 }
  };

  for (int i = 0; i < 8; ++i) {
    scan(map, player_x, player_y, 1, 1.0f, 0.0f, radius,
         multipliers[0][i], multipliers[1][i],
         multipliers[2][i], multipliers[3][i]);
  }
}

} // namespace Engine
