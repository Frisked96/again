#pragma once
#include <vector>

namespace GameMap {
class Map;
}

namespace Engine {

class FOV {
private:
  int width{0};
  int height{0};
  int radius{8};
  std::vector<bool> visible_grid;
  std::vector<bool> explored_grid;

  void scan(const GameMap::Map &map, int cx, int cy, int row,
            float start_slope, float end_slope, int current_radius,
            int xx, int xy, int yx, int yy);

public:
  FOV() = default;
  FOV(int w, int h, int r = 8);

  void resize(int w, int h);
  void compute(const GameMap::Map &map, int player_x, int player_y, int r = -1);

  bool is_visible(int x, int y) const;
  bool is_explored(int x, int y) const;

  void set_visible(int x, int y, bool visible);
  void set_explored(int x, int y, bool explored);

  void reveal_all();
  void reset();

  int get_radius() const { return radius; }
  void set_radius(int r) { radius = r; }

  int get_width() const { return width; }
  int get_height() const { return height; }
  bool in_bounds(int x, int y) const;
};

} // namespace Engine
