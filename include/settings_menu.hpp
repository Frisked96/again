#pragma once
#include "settings.hpp"
#include "terminal.hpp"
#include <string>
#include <string_view>

namespace Engine {

class SettingsMenu {
private:
  Terminal &terminal;
  Settings &settings;

  enum class FocusField {
    Width = 0,
    Height = 1,
    ManualEntry = 2,
    Save = 3,
    Cancel = 4
  };

  FocusField current_focus{FocusField::Width};

  std::string generate_frame(const Settings &temp_settings, int term_cols, int term_rows,
                             std::string_view message = "");
  bool prompt_numeric_input(std::string_view prompt_text, int current_val,
                            int min_val, int max_val, int &out_val);

public:
  SettingsMenu(Terminal &term, Settings &current_settings);

  // Opens the interactive settings menu. Returns true if changes were saved.
  bool run();
};

} // namespace Engine
