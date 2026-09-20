#include "settings.hpp"
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <format>
#include <fstream>
#include <sstream>

namespace Engine {

namespace {

void skip_whitespace(std::string_view str, size_t &pos) {
  while (pos < str.size() && std::isspace(static_cast<unsigned char>(str[pos]))) {
    ++pos;
  }
}

bool extract_int_after_key(std::string_view json_str, std::string_view key, int &out_val) {
  std::string needle = "\"" + std::string(key) + "\"";
  size_t found = json_str.find(needle);
  if (found == std::string_view::npos) {
    return false;
  }

  size_t pos = found + needle.size();
  skip_whitespace(json_str, pos);

  if (pos >= json_str.size() || json_str[pos] != ':') {
    return false;
  }
  ++pos; // skip ':'
  skip_whitespace(json_str, pos);

  if (pos >= json_str.size()) {
    return false;
  }

  bool neg = false;
  if (json_str[pos] == '-') {
    neg = true;
    ++pos;
  }

  if (pos >= json_str.size() || !std::isdigit(static_cast<unsigned char>(json_str[pos]))) {
    return false;
  }

  int val = 0;
  while (pos < json_str.size() && std::isdigit(static_cast<unsigned char>(json_str[pos]))) {
    val = val * 10 + (json_str[pos] - '0');
    ++pos;
  }

  out_val = neg ? -val : val;
  return true;
}

} // namespace

void Settings::clamp_to_terminal(int term_cols, int term_rows) {
  // Respect minimum bounds
  viewport_width = std::max(MIN_VIEWPORT_WIDTH, viewport_width);
  viewport_height = std::max(MIN_VIEWPORT_HEIGHT, viewport_height);

  // If valid terminal dimensions provided, clamp to prevent terminal overflow.
  // Game HUD and title take 6 lines, borders take 1-2 chars.
  if (term_cols > MIN_VIEWPORT_WIDTH) {
    int max_w = std::min(MAX_VIEWPORT_WIDTH, term_cols - 2);
    viewport_width = std::clamp(viewport_width, MIN_VIEWPORT_WIDTH, max_w);
  }
  if (term_rows > MIN_VIEWPORT_HEIGHT + 7) {
    int max_h = std::min(MAX_VIEWPORT_HEIGHT, term_rows - 7);
    viewport_height = std::clamp(viewport_height, MIN_VIEWPORT_HEIGHT, max_h);
  }

  fov_radius = std::clamp(fov_radius, 4, 32);
}

std::string Settings::to_json() const {
  std::ostringstream oss;
  oss << "{\n"
      << "  \"viewport_width\": " << viewport_width << ",\n"
      << "  \"viewport_height\": " << viewport_height << ",\n"
      << "  \"fov_radius\": " << fov_radius << "\n"
      << "}\n";
  return oss.str();
}

bool Settings::from_json(std::string_view json_str, Settings &out) {
  int w = 0;
  int h = 0;
  int fov = 8;

  bool has_w = extract_int_after_key(json_str, "viewport_width", w);
  bool has_h = extract_int_after_key(json_str, "viewport_height", h);
  extract_int_after_key(json_str, "fov_radius", fov);

  if (!has_w || !has_h) {
    return false;
  }

  out.viewport_width = std::clamp(w, MIN_VIEWPORT_WIDTH, MAX_VIEWPORT_WIDTH);
  out.viewport_height = std::clamp(h, MIN_VIEWPORT_HEIGHT, MAX_VIEWPORT_HEIGHT);
  out.fov_radius = std::clamp(fov, 4, 32);
  return true;
}

bool Settings::save(const std::string &path) const {
  std::ofstream file(path, std::ios::trunc);
  if (!file.is_open()) {
    return false;
  }
  file << to_json();
  return true;
}

Settings Settings::load_or_create(const std::string &path, int term_cols, int term_rows) {
  Settings settings;

  // Safe defaults based on terminal size
  int def_w = std::clamp(term_cols - 2, 40, 70);
  int def_h = std::clamp(term_rows - 7, 10, 18);
  settings.viewport_width = def_w;
  settings.viewport_height = def_h;
  settings.fov_radius = 8;

  std::error_code ec;
  if (!std::filesystem::exists(path, ec)) {
    // Auto-generate settings.json with default values
    settings.save(path);
    return settings;
  }

  std::ifstream file(path);
  if (!file.is_open()) {
    settings.save(path);
    return settings;
  }

  std::stringstream buffer;
  buffer << file.rdbuf();
  std::string content = buffer.str();

  Settings loaded;
  if (from_json(content, loaded)) {
    loaded.clamp_to_terminal(term_cols, term_rows);
    return loaded;
  }

  // Corrupted or invalid format: rewrite defaults
  settings.save(path);
  return settings;
}

} // namespace Engine
