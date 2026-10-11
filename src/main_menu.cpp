#include "main_menu.hpp"
#include "engine.hpp"
#include "settings_menu.hpp"
#include <algorithm>
#include <format>
#include <iostream>
#include <string>

namespace Engine {

MainMenu::MainMenu(Terminal &term, Settings &current_settings)
    : terminal(term), settings(current_settings) {}

void MainMenu::render_menu(int term_cols, int term_rows) {
  std::string buffer;
  buffer.reserve(2048);

  buffer.append("\033[H");

  int pad_width = std::max(40, std::min(term_cols, 80));

  auto center_line = [&](std::string_view text) {
    int len = static_cast<int>(text.size());
    int total_pad = std::max(0, pad_width - len);
    int left = total_pad / 2;
    buffer.append(std::string(left, ' '));
    buffer.append(text);
    buffer.append("\033[K\n");
  };

  buffer.append(std::string(pad_width, '='));
  buffer.append("\033[K\n");

  if (term_cols >= 72) {
    center_line(R"(    _    ____   ____ ___ ___  __        _____  ____  _     ____  )");
    center_line(R"(   / \  / ___| / ___|_ _|_ _| \ \      / / _ \|  _ \| |   |  _ \ )");
    center_line(R"(  / _ \ \___ \| |    | | | |   \ \ /\ / / | | | |_) | |   | | | |)");
    center_line(R"( / ___ \ ___) | |___ | | | |    \ V  V /| |_| |  _ <| |___| |_| |)");
    center_line(R"(/_/   \_\____/ \____|___|___|    \_/\_/  \___/|_| \_\_____|____/ )");
  } else {
    center_line("ASCII WORLD");
  }

  buffer.append(std::string(pad_width, '='));
  buffer.append("\033[K\n");
  center_line("A Vast Grim Fantasy Sandbox Realm");
  buffer.append(std::string(pad_width, '-'));
  buffer.append("\033[K\n\n");

  // Status subtitle
  std::string status_sub = std::format(
      "Configured Viewport: {} x {} | Terminal Size: {} x {}",
      settings.viewport_width, settings.viewport_height, term_cols, term_rows);
  center_line(status_sub);
  buffer.append("\n");

  // Menu items
  const char *items[] = {
      "[1]  Start Game",
      "[2]  Settings (Viewport & Preview)",
      "[3]  Quit"
  };

  for (int i = 0; i < 3; ++i) {
    std::string line;
    if (i == selected_index) {
      line = std::format("\033[1;33m  >  {}  <  \033[0m", items[i]);
    } else {
      line = std::format("\033[0;37m     {}     \033[0m", items[i]);
    }
    center_line(line);
  }

  buffer.append("\n");
  buffer.append(std::string(pad_width, '='));
  buffer.append("\033[K\n");
  center_line("[Up / Down / W / S] Navigate  |  [Enter / Space] Select  |  [Q] Quit");
  buffer.append(std::string(pad_width, '='));
  buffer.append("\033[K\033[J");

  terminal.present(buffer);
}

void MainMenu::launch_game() {
  Terminal::clear_screen();
  GameEngine game(terminal, 10000, 10000, settings.viewport_width, settings.viewport_height, settings.fov_radius);
  game.run();
  Terminal::clear_screen();
}

void MainMenu::open_settings() {
  Terminal::clear_screen();
  SettingsMenu menu(terminal, settings);
  menu.run();
  Terminal::clear_screen();
}

void MainMenu::run() {
  bool is_running = true;

  while (is_running) {
    int term_rows = 24;
    int term_cols = 80;
    Terminal::get_size(term_rows, term_cols);

    render_menu(term_cols, term_rows);

    RawKey key = terminal.read_key(-1);

    if (key.code == KeyCode::Escape) {
      is_running = false;
      break;
    }

    if (key.code == KeyCode::Up) {
      selected_index = (selected_index + 2) % 3;
    } else if (key.code == KeyCode::Down) {
      selected_index = (selected_index + 1) % 3;
    } else if (key.code == KeyCode::Enter || key.code == KeyCode::Space) {
      if (selected_index == 0) {
        launch_game();
      } else if (selected_index == 1) {
        open_settings();
      } else if (selected_index == 2) {
        is_running = false;
      }
    } else if (key.code == KeyCode::Char) {
      switch (key.ch) {
      case 'w':
      case 'W':
      case 'k':
      case 'K':
        selected_index = (selected_index + 2) % 3;
        break;
      case 's':
      case 'S':
      case 'j':
      case 'J':
        selected_index = (selected_index + 1) % 3;
        break;
      case '1':
        selected_index = 0;
        launch_game();
        break;
      case '2':
        selected_index = 1;
        open_settings();
        break;
      case '3':
      case 'q':
      case 'Q':
        is_running = false;
        break;
      default:
        break;
      }
    }
  }

  Terminal::clear_screen();
  terminal.write("Exited ASCII World. Goodbye!\n");
}

} // namespace Engine
