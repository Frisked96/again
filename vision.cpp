#include "vision.hpp"
#include "game_map.hpp"
#include "utils.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace Vision {

bool has_line_of_sight(const GameMap::Map &map, int x0, int y0, int x1, int y1, int max_range, int z) noexcept {
  if (!map.in_bounds(x0, y0) || !map.in_bounds(x1, y1)) {
    return false;
  }

  int dx_total = x1 - x0;
  int dy_total = y1 - y0;
  int dist_sq = dx_total * dx_total + dy_total * dy_total;
  int vis_limit = (max_range > 0) ? max_range : map.get_visibility_limit(x0, y0);
  if (dist_sq > vis_limit * vis_limit) {
    return false;
  }

  // Bresenham's line algorithm
  int dx = std::abs(dx_total);
  int dy = -std::abs(dy_total);
  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;
  int err = dx + dy;

  int curr_x = x0;
  int curr_y = y0;

  while (true) {
    if (curr_x == x1 && curr_y == y1) {
      return true; // Target reached without obstruction
    }

    if ((curr_x != x0 || curr_y != y0) && map.blocks_sight(curr_x, curr_y, z)) {
      return false; // Obstructed by wall, forest, or summit
    }

    int e2 = 2 * err;
    if (e2 >= dy) {
      err += dy;
      curr_x += sx;
    }
    if (e2 <= dx) {
      err += dx;
      curr_y += sy;
    }
  }
}

FOV::FOV(int w, int h, int r)
    : width(w), height(h), radius(r),
      visible_grid(w * h, false), explored_grid(w * h, false) {}

void FOV::resize(int w, int h) {
  width = w;
  height = h;
  visible_grid.assign(w * h, false);
  explored_grid.assign(w * h, false);
  visible_cells.clear();
}

bool FOV::in_bounds(int x, int y) const noexcept {
  return x >= 0 && x < width && y >= 0 && y < height;
}

bool FOV::is_visible(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return false;
  }
  return visible_grid[Utils::to_index(x, y, width)];
}

bool FOV::is_explored(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return false;
  }
  return explored_grid[Utils::to_index(x, y, width)];
}

void FOV::set_visible(int x, int y, bool visible) noexcept {
  if (in_bounds(x, y)) {
    visible_grid[Utils::to_index(x, y, width)] = visible;
  }
}

void FOV::set_explored(int x, int y, bool explored) noexcept {
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
  visible_cells.clear();
}

void FOV::scan(const GameMap::Map &map, int cx, int cy, int cz, int row,
               float start_slope, float end_slope, int current_radius,
               int xx, int xy, int yx, int yy) {
  if (start_slope < end_slope) {
    return;
  }

  float next_start_slope = start_slope;

  for (int j = row; j <= current_radius; ++j) {
    int dy = -j;
    bool blocked = false;

    for (int dx = -j; dx <= 0; ++dx) {
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
          if (!visible_grid[index]) {
            visible_grid[index] = true;
            visible_cells.push_back(index);
          }
          explored_grid[index] = true;
        }
      }

      bool is_blocking = !map.in_bounds(X, Y) || map.blocks_sight(X, Y, cz);

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
          scan(map, cx, cy, cz, j + 1, start_slope, l_slope, current_radius,
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

void FOV::compute(const GameMap::Map &map, int center_x, int center_y, int center_z, int r) {
  if (r > 0) {
    radius = r;
  }

  // Clear previous turn's visible cells efficiently in O(visible_count)
  for (int idx : visible_cells) {
    visible_grid[idx] = false;
  }
  visible_cells.clear();

  // Center tile is always visible and explored
  if (in_bounds(center_x, center_y)) {
    int idx = Utils::to_index(center_x, center_y, width);
    visible_grid[idx] = true;
    explored_grid[idx] = true;
    visible_cells.push_back(idx);
  }

  // 8 octant multipliers
  static const int multipliers[4][8] = {
      { 1,  0,  0, -1, -1,  0,  0,  1 },
      { 0,  1, -1,  0,  0, -1,  1,  0 },
      { 0,  1,  1,  0,  0, -1, -1,  0 },
      { 1,  0,  0,  1, -1,  0,  0, -1 }
  };

  for (int i = 0; i < 8; ++i) {
    scan(map, center_x, center_y, center_z, 1, 1.0f, 0.0f, radius,
         multipliers[0][i], multipliers[1][i],
         multipliers[2][i], multipliers[3][i]);
  }
}

} // namespace Vision
