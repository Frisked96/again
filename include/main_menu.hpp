#pragma once
#include "settings.hpp"
#include "terminal.hpp"

namespace Engine {

class MainMenu {
public:
  enum class MenuOption {
    StartGame = 0,
    Settings = 1,
    Quit = 2
  };

private:
  Terminal &terminal;
  Settings &settings;
  int selected_index{0};

  void render_menu(int term_cols, int term_rows);
  void launch_game();
  void open_settings();

public:
  MainMenu(Terminal &term, Settings &current_settings);

  void run();
};

} // namespace Engine
