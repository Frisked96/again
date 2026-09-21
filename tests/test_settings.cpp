#include "test_common.hpp"
#include "../settings.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

void test_clamping() {
  Engine::Settings s;
  s.viewport_width = 1000;
  s.viewport_height = 500;
  s.fov_radius = 100;

  s.clamp_to_terminal(80, 24);
  CHECK(s.viewport_width <= 78);
  CHECK(s.viewport_width >= Engine::Settings::MIN_VIEWPORT_WIDTH);
  CHECK(s.viewport_height <= 17);
  CHECK(s.viewport_height >= Engine::Settings::MIN_VIEWPORT_HEIGHT);
  CHECK(s.fov_radius <= 32);

  // Test lower bounds
  s.viewport_width = 2;
  s.viewport_height = 2;
  s.fov_radius = 1;
  s.clamp_to_terminal(80, 24);
  CHECK(s.viewport_width == Engine::Settings::MIN_VIEWPORT_WIDTH);
  CHECK(s.viewport_height == Engine::Settings::MIN_VIEWPORT_HEIGHT);
  CHECK(s.fov_radius >= 4);
}

void test_json_roundtrip() {
  Engine::Settings original;
  original.viewport_width = 65;
  original.viewport_height = 15;
  original.fov_radius = 10;

  std::string json = original.to_json();
  CHECK(!json.empty());
  CHECK(json.find("\"viewport_width\": 65") != std::string::npos);
  CHECK(json.find("\"viewport_height\": 15") != std::string::npos);

  Engine::Settings parsed;
  bool ok = Engine::Settings::from_json(json, parsed);
  CHECK(ok);
  CHECK(parsed.viewport_width == 65);
  CHECK(parsed.viewport_height == 15);
  CHECK(parsed.fov_radius == 10);
}

void test_auto_generate_and_persistence() {
  const std::string test_path = "test_run_settings.json";
  std::filesystem::remove(test_path);

  // 1. Auto-generate when non-existent
  CHECK(!std::filesystem::exists(test_path));
  Engine::Settings s1 = Engine::Settings::load_or_create(test_path, 80, 24);
  CHECK(std::filesystem::exists(test_path));
  CHECK(s1.viewport_width > 0);
  CHECK(s1.viewport_height > 0);

  // 2. Modify and save
  s1.viewport_width = 55;
  s1.viewport_height = 14;
  bool saved = s1.save(test_path);
  CHECK(saved);

  // 3. Reload from disk
  Engine::Settings s2 = Engine::Settings::load_or_create(test_path, 80, 24);
  CHECK(s2.viewport_width == 55);
  CHECK(s2.viewport_height == 14);

  // 4. Corrupt file recovery
  {
    std::ofstream corrupt(test_path, std::ios::trunc);
    corrupt << "{ invalid_json: true, unexpected token !!! }";
  }
  Engine::Settings s3 = Engine::Settings::load_or_create(test_path, 80, 24);
  CHECK(s3.viewport_width >= Engine::Settings::MIN_VIEWPORT_WIDTH);
  CHECK(s3.viewport_height >= Engine::Settings::MIN_VIEWPORT_HEIGHT);

  std::filesystem::remove(test_path);
}

int main() {
  test_clamping();
  test_json_roundtrip();
  test_auto_generate_and_persistence();

  std::cout << "All settings tests passed.\n";
  return 0;
}
