#include "renderer.hpp"
#include "terminal.hpp"
#include "tile.hpp"
#include <algorithm>
#include <format>
#include <string>

namespace Engine {

void Renderer::render(const GameMap::Map &map,
                      const Entity &player,
                      const Camera &camera,
                      const FOV &fov,
                      Terminal &terminal) {
  std::string buffer;
  int view_w = camera.get_viewport_width();
  int view_h = camera.get_viewport_height();
  int cam_x = camera.get_x();
  int cam_y = camera.get_y();

  // Reserve estimated space: header lines + map characters + footer lines
  buffer.reserve((view_w + 32) * view_h + 512);

  // Reposition cursor to top-left (home)
  buffer.append("\033[H");

  // Title header centered
  std::string title = " ASCII WORLD ";
  int total_pad = std::max(0, view_w - static_cast<int>(title.size()));
  int left_pad = total_pad / 2;
  int right_pad = total_pad - left_pad;
  buffer.append(std::string(left_pad, '='));
  buffer.append(title);
  buffer.append(std::string(right_pad, '='));
  buffer.push_back('\n');

  // Tracking ANSI color styling to minimize escape sequences
  enum class Style { None, Normal, Dim, Player };
  Style current_style = Style::None;

  auto set_style = [&](Style new_style) {
    if (current_style == new_style) {
      return;
    }
    switch (new_style) {
    case Style::Normal:
      buffer.append("\033[0m");
      break;
    case Style::Dim:
      buffer.append("\033[0;90m"); // Dark gray for explored fog of war
      break;
    case Style::Player:
      buffer.append("\033[1;33m"); // Bright yellow for player
      break;
    case Style::None:
      buffer.append("\033[0m");
      break;
    }
    current_style = new_style;
  };

  // Render viewport grid
  for (int vy = 0; vy < view_h; ++vy) {
    for (int vx = 0; vx < view_w; ++vx) {
      int wx = cam_x + vx;
      int wy = cam_y + vy;

      if (!map.in_bounds(wx, wy)) {
        set_style(Style::Normal);
        buffer.push_back(' ');
        continue;
      }

      if (fov.is_visible(wx, wy)) {
        if (wx == player.x && wy == player.y) {
          set_style(Style::Player);
          buffer.push_back(player.glyph);
        } else {
          set_style(Style::Normal);
          buffer.push_back(Tile::getData(map.at(wx, wy)).glyph);
        }
      } else if (fov.is_explored(wx, wy)) {
        set_style(Style::Dim);
        buffer.push_back(Tile::getData(map.at(wx, wy)).glyph);
      } else {
        set_style(Style::Normal);
        buffer.push_back(' ');
      }
    }
    set_style(Style::Normal);
    buffer.push_back('\n');
  }
  set_style(Style::Normal);

  // HUD & Status
  auto current_tile = Tile::getData(map.at(player.x, player.y));
  buffer.append(std::string(view_w, '-'));
  buffer.push_back('\n');
  buffer.append(std::format("Position: ({}, {}) | Standing on: {}\n",
                            player.x, player.y, current_tile.name));
  buffer.append(std::format("Camera: ({}, {}) | FOV Radius: {}\n",
                            cam_x, cam_y, fov.get_radius()));
  buffer.append("Controls: [WASD / Arrows / HJKL] Move | [Q] Quit\n");
  buffer.append(std::string(view_w, '-'));
  buffer.push_back('\n');

  terminal.present(buffer);
}

} // namespace Engine
