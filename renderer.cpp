#include "renderer.hpp"
#include "tile.hpp"
#include <algorithm>
#include <format>
#include <string>

namespace Engine {

std::string Renderer::render(const GameMap::Map &map,
                             const Entity &player,
                             const Camera &camera,
                             const Vision::FOV &fov,
                             std::string_view status_message) {
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
  buffer.append("\033[K\n");

  // Tracking ANSI color styling to minimize escape sequences
  enum class Style { None, Normal, Dim, Player, Flora, FloraDim, Structure, StructureDim };
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
    case Style::Flora:
      buffer.append("\033[1;32m"); // Bright green for standing flora
      break;
    case Style::FloraDim:
      buffer.append("\033[0;32m"); // Dim green for explored flora
      break;
    case Style::Structure:
      buffer.append("\033[1;33m"); // Warm amber for doors & architectural features
      break;
    case Style::StructureDim:
      buffer.append("\033[0;33m"); // Dim amber for explored doors
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
          auto veg = map.get_vegetation(wx, wy);
          if (veg.id != Vegetation::ID::None) {
            set_style(Style::Flora);
            buffer.push_back(Vegetation::getData(veg.id).glyph);
          } else {
            Tile::ID tid = map.at(wx, wy);
            if (tid == Tile::ID::DoorClosed || tid == Tile::ID::DoorOpen || tid == Tile::ID::StairsDown) {
              set_style(Style::Structure);
            } else {
              set_style(Style::Normal);
            }
            buffer.push_back(Tile::getData(tid).glyph);
          }
        }
      } else if (fov.is_explored(wx, wy)) {
        auto veg = map.get_vegetation(wx, wy);
        if (veg.id != Vegetation::ID::None) {
          set_style(Style::FloraDim);
          buffer.push_back(Vegetation::getData(veg.id).glyph);
        } else {
          Tile::ID tid = map.at(wx, wy);
          if (tid == Tile::ID::DoorClosed || tid == Tile::ID::DoorOpen || tid == Tile::ID::StairsDown) {
            set_style(Style::StructureDim);
          } else {
            set_style(Style::Dim);
          }
          buffer.push_back(Tile::getData(tid).glyph);
        }
      } else {
        set_style(Style::Normal);
        buffer.push_back(' ');
      }
    }
    set_style(Style::Normal);
    buffer.append("\033[K\n");
  }
  set_style(Style::Normal);

  // HUD & Status
  auto current_tile = Tile::getData(map.at(player.x, player.y));
  auto current_veg = map.get_vegetation(player.x, player.y);
  auto current_region = World::getRegionData(map.get_region(player.x, player.y));
  auto current_weather = map.get_weather(player.x, player.y);

  buffer.append(std::string(view_w, '-'));
  buffer.append("\033[K\n");

  if (current_veg.id != Vegetation::ID::None) {
    auto vdata = Vegetation::getData(current_veg.id);
    buffer.append(std::format("Pos: ({}, {}) | Flora: {} [{}: {}%] over {}",
                              player.x, player.y, vdata.name,
                              Vegetation::resource_name(vdata.resourceType),
                              current_veg.resource_amount, current_tile.name));
  } else {
    buffer.append(std::format("Pos: ({}, {}) | Ground: {}",
                              player.x, player.y, current_tile.name));
  }
  buffer.append("\033[K\n");

  buffer.append(std::format("Region: {} | Climate: {} ({:.1f}°C)",
                            current_region.name, current_weather.name,
                            current_weather.temperature_celsius));
  buffer.append("\033[K\n");

  // Fixed 1-line action / control bar to prevent height oscillation
  if (!status_message.empty()) {
    buffer.append(std::format(">> {}", status_message));
  } else {
    buffer.append("Controls: [WASD / Arrows] Move | [E / Space] Harvest | [Q] Quit");
  }
  buffer.append("\033[K\n");

  // Bottom border without trailing newline to avoid scrolling on row 24
  buffer.append(std::string(view_w, '-'));
  buffer.append("\033[K\033[J");

  return buffer;
}

} // namespace Engine
