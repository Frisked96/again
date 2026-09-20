#pragma once
#include <string>
#include <string_view>

namespace Engine {

struct Settings {
  int viewport_width{50};
  int viewport_height{18};
  int fov_radius{8};

  static constexpr int MIN_VIEWPORT_WIDTH = 20;
  static constexpr int MIN_VIEWPORT_HEIGHT = 8;
  static constexpr int MAX_VIEWPORT_WIDTH = 240;
  static constexpr int MAX_VIEWPORT_HEIGHT = 100;
  static constexpr const char *DEFAULT_SETTINGS_PATH = "settings.json";

  // Clamps width and height within terminal boundaries to avoid terminal scrolling
  void clamp_to_terminal(int term_cols, int term_rows);

  // Persistence
  static Settings load_or_create(const std::string &path = DEFAULT_SETTINGS_PATH,
                                 int term_cols = 80, int term_rows = 24);
  bool save(const std::string &path = DEFAULT_SETTINGS_PATH) const;
  std::string to_json() const;
  static bool from_json(std::string_view json_str, Settings &out);
};

} // namespace Engine
