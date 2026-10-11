#pragma once

namespace Utils {
// Converts 2D coordinates to a 1D index
inline constexpr int to_index(int x, int y, int width) {
  return (y * width) + x;
}

// Converts a 1D index back to 2D coordinates
struct Coords {
  int x;
  int y;
};
inline constexpr Coords to_coords(int index, int width) {
  return {index % width, index / width};
}
} // namespace Utils
