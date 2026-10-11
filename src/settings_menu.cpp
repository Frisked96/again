#include "settings_menu.hpp"
#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace Engine {

SettingsMenu::SettingsMenu(Terminal &term, Settings &current_settings)
    : terminal(term), settings(current_settings) {}

bool SettingsMenu::prompt_numeric_input(std::string_view prompt_text, int current_val,
                                        int min_val, int max_val, int &out_val) {
  std::string input_buffer;

  while (true) {
    std::string prompt_frame = "\033[H\033[2K\033[1;33m== MANUAL ENTRY ==\033[0m\n\033[2K";
    prompt_frame.append(prompt_text);
    prompt_frame.append(std::format(" [{}-{}, current: {}]: ", min_val, max_val, current_val));
    prompt_frame.append(input_buffer);
    prompt_frame.append("_\033[K\n\033[2K\033[90m(Press Enter to confirm, Esc to cancel, Backspace to delete)\033[0m\033[K");
    terminal.present(prompt_frame);

    RawKey key = terminal.read_key(-1);
    if (key.code == KeyCode::Escape) {
      return false;
    }
    if (key.code == KeyCode::Enter) {
      if (input_buffer.empty()) {
        out_val = current_val;
        return true;
      }
      try {
        int parsed = std::stoi(input_buffer);
        out_val = std::clamp(parsed, min_val, max_val);
        return true;
      } catch (...) {
        input_buffer.clear();
      }
    } else if (key.code == KeyCode::Backspace) {
      if (!input_buffer.empty()) {
        input_buffer.pop_back();
      }
    } else if (key.code == KeyCode::Char && std::isdigit(static_cast<unsigned char>(key.ch))) {
      if (input_buffer.size() < 4) {
        input_buffer.push_back(key.ch);
      }
    }
  }
}

std::string SettingsMenu::generate_frame(const Settings &temp_settings, int term_cols, int term_rows,
                                         std::string_view status_msg) {
  std::string buffer;
  int view_w = temp_settings.viewport_width;
  int view_h = temp_settings.viewport_height;
  int footprint_h = view_h + 6; // header (1) + map (view_h) + HUD (4) + bottom border (1)

  buffer.reserve((view_w + 32) * (view_h + 8) + 1024);

  // Home cursor
  buffer.append("\033[H");

  // Top header & interactive controls bar
  buffer.append("\033[1;36m== SETTINGS ==\033[0m ");

  auto format_tab = [&](FocusField field, std::string_view label, std::string_view val) {
    if (current_focus == field) {
      return std::format("\033[1;33;44m [ {} ] \033[0m ", val.empty() ? label : std::format("{}: < {} >", label, val));
    }
    return std::format("\033[0;37m [ {} ] \033[0m ", val.empty() ? label : std::format("{}: < {} >", label, val));
  };

  buffer.append(format_tab(FocusField::Width, "1:Width", std::to_string(view_w)));
  buffer.append(format_tab(FocusField::Height, "2:Height", std::to_string(view_h)));
  buffer.append(format_tab(FocusField::ManualEntry, "3:Manual [M]", ""));
  buffer.append(format_tab(FocusField::Save, "4:Save [Enter/S]", ""));
  buffer.append(format_tab(FocusField::Cancel, "5:Cancel [Esc]", ""));
  buffer.append("\033[K\n");

  // Status metrics & boundary fitting line
  int margin_w = term_cols - view_w;
  int margin_h = term_rows - footprint_h - 2; // remaining rows after menu header (2 lines)

  std::string fit_badge;
  if (margin_w >= 0 && margin_h >= 0) {
    fit_badge = "\033[1;32m[OK] FITS SAFELY\033[0m";
  } else {
    fit_badge = "\033[1;31m[WARN] EXCEEDS TERMINAL\033[0m";
  }

  buffer.append(std::format(
      "Terminal: {}x{} | Viewport: {}x{} | Footprint: {}x{} | Margin: {:+d} cols, {:+d} rows | {}",
      term_cols, term_rows, view_w, view_h, view_w, footprint_h, margin_w, margin_h, fit_badge));
  if (!status_msg.empty()) {
    buffer.append(std::format(" | \033[1;33m{}\033[0m", status_msg));
  }
  buffer.append("\033[K\n");

  // Viewport Preview Box
  // 1. Centered ASCII WORLD Title Header
  std::string title = " ASCII WORLD (PREVIEW) ";
  int total_pad = std::max(0, view_w - static_cast<int>(title.size()));
  int left_pad = total_pad / 2;
  int right_pad = total_pad - left_pad;
  buffer.append(std::string(left_pad, '='));
  buffer.append(title);
  buffer.append(std::string(right_pad, '='));
  buffer.append("\033[K\n");

  // 2. Procedural Sample Terrain Grid centered around Player
  int cx = view_w / 2;
  int cy = view_h / 2;

  for (int vy = 0; vy < view_h; ++vy) {
    for (int vx = 0; vx < view_w; ++vx) {
      if (vx == cx && vy == cy) {
        // Player glyph in bright yellow
        buffer.append("\033[1;33m@\033[0m");
        continue;
      }

      // Elliptical distance for FOV (terminal characters are roughly twice as tall as wide)
      double dx = vx - cx;
      double dy = (vy - cy) * 1.8;
      double dist = std::sqrt(dx * dx + dy * dy);
      bool is_visible = dist <= 8.5;
      bool is_explored = dist <= 14.0;

      // Sample features: road, river, trees, flora, ground
      char glyph = '.';
      std::string_view style = "\033[0m"; // normal

      if (vx == cx - 3 || (vy == cy + 2 && vx >= cx - 3 && vx <= cx + 4)) {
        // Cobblestone road
        glyph = '#';
        style = is_visible ? "\033[1;37m" : (is_explored ? "\033[0;90m" : "\033[0m");
      } else if (vx == cx + 5 && vy == cy - 1) {
        // Wooden door of a cottage
        glyph = '+';
        style = is_visible ? "\033[1;33m" : (is_explored ? "\033[0;33m" : "\033[0m");
      } else if ((vx >= cx + 4 && vx <= cx + 7) && (vy >= cy - 3 && vy <= cy - 1)) {
        // Cabin wall
        glyph = '#';
        style = is_visible ? "\033[0;37m" : (is_explored ? "\033[0;90m" : "\033[0m");
      } else if ((vx + vy * 2) % 19 == 0) {
        // River / stream
        glyph = '~';
        style = is_visible ? "\033[1;36m" : (is_explored ? "\033[0;36m" : "\033[0m");
      } else if ((vx * 7 + vy * 13) % 11 == 0) {
        // Berry shrub
        glyph = '&';
        style = is_visible ? "\033[1;32m" : (is_explored ? "\033[0;32m" : "\033[0m");
      } else if ((vx * 3 + vy * 5) % 7 == 0) {
        // Oak / Pine Tree
        glyph = 'T';
        style = is_visible ? "\033[1;32m" : (is_explored ? "\033[0;32m" : "\033[0m");
      } else {
        // Ground substrate
        glyph = '.';
        style = is_visible ? "\033[0m" : (is_explored ? "\033[0;90m" : "\033[0m");
      }

      if (!is_explored) {
        buffer.push_back(' ');
      } else {
        buffer.append(style);
        buffer.push_back(glyph);
      }
    }
    buffer.append("\033[0m\033[K\n");
  }

  // 3. HUD Separator & Sample Status Lines
  buffer.append(std::string(view_w, '-'));
  buffer.append("\033[K\n");

  std::string hud1 = "Pos: (5000, 5000) | Flora: berry shrub [Yield: 30%] over forest loam";
  if (static_cast<int>(hud1.size()) > view_w) {
    hud1 = hud1.substr(0, std::max(0, view_w));
  }
  buffer.append(hud1);
  buffer.append("\033[K\n");

  std::string hud2 = "Region: The Oldwood Weald | Climate: Overcast (1.3°C)";
  if (static_cast<int>(hud2.size()) > view_w) {
    hud2 = hud2.substr(0, std::max(0, view_w));
  }
  buffer.append(hud2);
  buffer.append("\033[K\n");

  std::string hud3 = "Controls: [WASD / Arrows] Move | [E / Space] Harvest | [Q] Quit";
  if (static_cast<int>(hud3.size()) > view_w) {
    hud3 = hud3.substr(0, std::max(0, view_w));
  }
  buffer.append(hud3);
  buffer.append("\033[K\n");

  buffer.append(std::string(view_w, '-'));
  buffer.append("\033[K\n");

  // 4. Instructions and Dimension Bar
  buffer.append(std::format(
      "\033[90mDimensions: Width: {} cols | Height: {} rows | Keys: [◄/► or A/D] Adjust | [Tab/▲/▼] Focus | [M] Manual | [S] Save\033[0m",
      view_w, view_h));
  buffer.append("\033[K\033[J");

  return buffer;
}

bool SettingsMenu::run() {
  Settings temp = settings;
  std::string status_msg;

  while (true) {
    int term_rows = 24;
    int term_cols = 80;
    Terminal::get_size(term_rows, term_cols);

    // Calculate maximum viewport that fits comfortably in this terminal
    int max_w = std::min(Settings::MAX_VIEWPORT_WIDTH, std::max(Settings::MIN_VIEWPORT_WIDTH, term_cols - 2));
    int max_h = std::min(Settings::MAX_VIEWPORT_HEIGHT, std::max(Settings::MIN_VIEWPORT_HEIGHT, term_rows - 8));

    // Clamp working values to terminal limits
    temp.viewport_width = std::clamp(temp.viewport_width, Settings::MIN_VIEWPORT_WIDTH, max_w);
    temp.viewport_height = std::clamp(temp.viewport_height, Settings::MIN_VIEWPORT_HEIGHT, max_h);

    std::string frame = generate_frame(temp, term_cols, term_rows, status_msg);
    terminal.present(frame);
    status_msg.clear();

    RawKey key = terminal.read_key(-1);

    if (key.code == KeyCode::Escape) {
      // Discard and exit
      return false;
    }

    if (key.code == KeyCode::Left) {
      if (current_focus == FocusField::Height) {
        temp.viewport_height = std::max(Settings::MIN_VIEWPORT_HEIGHT, temp.viewport_height - 1);
      } else {
        temp.viewport_width = std::max(Settings::MIN_VIEWPORT_WIDTH, temp.viewport_width - 1);
      }
    } else if (key.code == KeyCode::Right) {
      if (current_focus == FocusField::Height) {
        temp.viewport_height = std::min(max_h, temp.viewport_height + 1);
      } else {
        temp.viewport_width = std::min(max_w, temp.viewport_width + 1);
      }
    } else if (key.code == KeyCode::Up) {
      current_focus = static_cast<FocusField>((static_cast<int>(current_focus) + 4) % 5);
    } else if (key.code == KeyCode::Down) {
      current_focus = static_cast<FocusField>((static_cast<int>(current_focus) + 1) % 5);
    } else if (key.code == KeyCode::Enter) {
      if (current_focus == FocusField::Width) {
        int val = temp.viewport_width;
        if (prompt_numeric_input("Enter Viewport Width", val, Settings::MIN_VIEWPORT_WIDTH, max_w, val)) {
          temp.viewport_width = val;
          status_msg = "Width updated!";
        }
      } else if (current_focus == FocusField::Height) {
        int val = temp.viewport_height;
        if (prompt_numeric_input("Enter Viewport Height", val, Settings::MIN_VIEWPORT_HEIGHT, max_h, val)) {
          temp.viewport_height = val;
          status_msg = "Height updated!";
        }
      } else if (current_focus == FocusField::ManualEntry) {
        int w = temp.viewport_width;
        int h = temp.viewport_height;
        if (prompt_numeric_input("Enter Viewport Width", w, Settings::MIN_VIEWPORT_WIDTH, max_w, w)) {
          temp.viewport_width = w;
          if (prompt_numeric_input("Enter Viewport Height", h, Settings::MIN_VIEWPORT_HEIGHT, max_h, h)) {
            temp.viewport_height = h;
            status_msg = "Viewport dimensions updated!";
          }
        }
      } else if (current_focus == FocusField::Save) {
        settings = temp;
        settings.save();
        return true;
      } else if (current_focus == FocusField::Cancel) {
        return false;
      }
    } else if (key.code == KeyCode::Char) {
      switch (key.ch) {
      case '1':
        current_focus = FocusField::Width;
        break;
      case '2':
        current_focus = FocusField::Height;
        break;
      case '3':
      case 'm':
      case 'M': {
        current_focus = FocusField::ManualEntry;
        int w = temp.viewport_width;
        int h = temp.viewport_height;
        if (prompt_numeric_input("Enter Viewport Width", w, Settings::MIN_VIEWPORT_WIDTH, max_w, w)) {
          temp.viewport_width = w;
          if (prompt_numeric_input("Enter Viewport Height", h, Settings::MIN_VIEWPORT_HEIGHT, max_h, h)) {
            temp.viewport_height = h;
            status_msg = "Viewport dimensions updated!";
          }
        }
        break;
      }
      case '4':
      case 's':
      case 'S':
        settings = temp;
        settings.save();
        return true;
      case '5':
      case 'q':
      case 'Q':
        return false;
      case 'a':
      case 'A':
      case '-':
      case '[':
        temp.viewport_width = std::max(Settings::MIN_VIEWPORT_WIDTH, temp.viewport_width - 1);
        break;
      case 'd':
      case 'D':
      case '+':
      case '=':
      case ']':
        temp.viewport_width = std::min(max_w, temp.viewport_width + 1);
        break;
      case 'w':
      case 'W':
      case '{':
        temp.viewport_height = std::min(max_h, temp.viewport_height + 1);
        break;
      case 'j':
      case '}':
        temp.viewport_height = std::max(Settings::MIN_VIEWPORT_HEIGHT, temp.viewport_height - 1);
        break;
      case '\t':
        current_focus = static_cast<FocusField>((static_cast<int>(current_focus) + 1) % 5);
        break;
      default:
        break;
      }
    }
  }
}

} // namespace Engine
