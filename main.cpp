#include "main_menu.hpp"
#include "settings.hpp"
#include "terminal.hpp"

int main() {
  Engine::Terminal terminal;

  int term_rows = 24;
  int term_cols = 80;
  Engine::Terminal::get_size(term_rows, term_cols);

  // Auto-generate settings.json if non-existent, or load existing configuration
  Engine::Settings settings =
      Engine::Settings::load_or_create(Engine::Settings::DEFAULT_SETTINGS_PATH, term_cols, term_rows);

  Engine::MainMenu menu(terminal, settings);
  menu.run();

  return 0;
}
