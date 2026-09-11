#include "renderer.hpp"
#include "tile.hpp"
#include <iostream>
#include <string>

namespace Engine {

void Renderer::render(const GameMap::map &map, const Entity &player) {
  std::string buffer;
  // Reserve estimated space: header lines + map characters + footer lines
  int width = map.get_width();
  int height = map.get_height();
  buffer.reserve((width + 1) * height + 512);

  // Reposition cursor to top-left (home)
  buffer.append("\033[H");

  // Title header
  buffer.append("=================== ASCII WORLD ===================\n");

  // Render map grid
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      if (x == player.x && y == player.y) {
        buffer.push_back(player.glyph);
      } else {
        buffer.push_back(Tile::getData(map.at(x, y)).glyph);
      }
    }
    buffer.push_back('\n');
  }

  // HUD & Status
  auto current_tile = Tile::getData(map.at(player.x, player.y));
  buffer.append("---------------------------------------------------\n");
  buffer.append("Position: (" + std::to_string(player.x) + ", " +
                std::to_string(player.y) + ") | Standing on: " +
                std::string(current_tile.name) + "\n");
  buffer.append("Controls: [WASD / Arrows / HJKL] Move | [Q] Quit\n");
  buffer.append("---------------------------------------------------\n");

  std::cout << buffer << std::flush;
}

} // namespace Engine
