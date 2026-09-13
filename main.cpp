#include "engine.hpp"
#include "terminal.hpp"
#include <algorithm>

int main() {
  int term_rows = 24;
  int term_cols = 80;
  Engine::Terminal::get_size(term_rows, term_cols);

  // Dynamic viewport sizing fitting safely within terminal boundaries to prevent scrolling
  int view_w = std::clamp(term_cols - 2, 40, 70);
  int view_h = std::clamp(term_rows - 7, 10, 18);

  Engine::GameEngine game(10000, 10000, view_w, view_h, 8);
  game.run();
  return 0;
}
