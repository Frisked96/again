#pragma once
#include <vector>

namespace GameMap {
class Map;
}

namespace Vision {

// Point-to-point Line of Sight (Bresenham Raycasting)
// If max_range < 0, checks against weather visibility limit at the origin cell
bool has_line_of_sight(const GameMap::Map &map, int x0, int y0, int x1, int y1, int max_range = -1) noexcept;

// 2D Field of View and Exploration Memory (Octant Shadowcasting)
class FOV {
private:
  int width{0};
  int height{0};
  int radius{8};
  std::vector<bool> visible_grid;
  std::vector<bool> explored_grid;
  std::vector<int> visible_cells;

  void scan(const GameMap::Map &map, int cx, int cy, int row,
            float start_slope, float end_slope, int current_radius,
            int xx, int xy, int yx, int yy);

public:
  FOV() = default;
  FOV(int w, int h, int r = 8);

  void resize(int w, int h);
  void compute(const GameMap::Map &map, int center_x, int center_y, int r = -1);

  [[nodiscard]] bool is_visible(int x, int y) const noexcept;
  [[nodiscard]] bool is_explored(int x, int y) const noexcept;

  void set_visible(int x, int y, bool visible) noexcept;
  void set_explored(int x, int y, bool explored) noexcept;

  void reveal_all();
  void reset();

  [[nodiscard]] int get_radius() const noexcept { return radius; }
  void set_radius(int r) noexcept { radius = r; }

  [[nodiscard]] int get_width() const noexcept { return width; }
  [[nodiscard]] int get_height() const noexcept { return height; }
  [[nodiscard]] bool in_bounds(int x, int y) const noexcept;
};

} // namespace Vision
