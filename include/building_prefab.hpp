#pragma once
#include "structure.hpp"
#include "tile.hpp"
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Architecture {

struct RoomDescriptor {
  std::string name;
  int x1{0};
  int y1{0};
  int x2{0};
  int y2{0};

  [[nodiscard]] bool contains(int local_x, int local_y) const noexcept {
    return local_x >= x1 && local_x <= x2 && local_y >= y1 && local_y <= y2;
  }
};

struct BuildingFloor {
  int level{0}; // 0 = ground, 1 = upper, -1 = cellar
  std::string name{"Ground Floor"};
  Structure::ID wall_type{Structure::ID::WoodWall};
  Structure::ID floor_type{Structure::ID::WoodFloor};
  std::vector<std::string> layout; // ASCII lines of width characters
  std::vector<RoomDescriptor> rooms;

  [[nodiscard]] bool is_valid(int expected_w, int expected_h) const noexcept {
    if (layout.size() != static_cast<size_t>(expected_h)) return false;
    for (const auto &row : layout) {
      if (static_cast<int>(row.size()) != expected_w) return false;
    }
    return true;
  }

  [[nodiscard]] std::string_view get_room_name(int local_x, int local_y) const noexcept {
    for (const auto &room : rooms) {
      if (room.contains(local_x, local_y)) {
        return room.name;
      }
    }
    return name.empty() ? std::string_view{"Interior"} : std::string_view{name};
  }
};

struct BuildingTemplate {
  std::string id;
  std::string name;
  std::string category{"residential"}; // "residential", "commercial", "religious", "military"
  std::vector<std::string> settlement_tiers; // "hamlet", "village", "town", "city"
  std::vector<std::string> biomes;
  int width{0};
  int height{0};
  Structure::ID wall_type{Structure::ID::WoodWall};
  Structure::ID floor_type{Structure::ID::WoodFloor};
  std::vector<std::string> layout; // ASCII lines of width characters (Level 0)
  std::vector<RoomDescriptor> rooms;
  std::vector<BuildingFloor> floors; // Multi-level floor representations

  [[nodiscard]] bool is_valid() const noexcept {
    if (width <= 0 || height <= 0) return false;
    if (!floors.empty()) {
      for (const auto &f : floors) {
        if (!f.is_valid(width, height)) return false;
      }
      return true;
    }
    return layout.size() == static_cast<size_t>(height);
  }

  [[nodiscard]] const BuildingFloor* get_floor(int z) const noexcept {
    for (const auto &f : floors) {
      if (f.level == z) return &f;
    }
    return nullptr;
  }

  [[nodiscard]] std::string_view get_room_name(int local_x, int local_y, int z = 0) const noexcept {
    if (const auto *fl = get_floor(z)) {
      return fl->get_room_name(local_x, local_y);
    }
    for (const auto &room : rooms) {
      if (room.contains(local_x, local_y)) {
        return room.name;
      }
    }
    return "Interior";
  }
};

class PrefabCatalog {
private:
  std::vector<BuildingTemplate> templates;

public:
  PrefabCatalog() = default;

  // Loads templates from JSON file; if file cannot be read, falls back to embedded defaults
  bool load(const std::string &filepath = "data/prefabs/buildings.json");
  bool load_from_string(std::string_view json_str);

  [[nodiscard]] const std::vector<BuildingTemplate>& get_all() const noexcept {
    return templates;
  }

  [[nodiscard]] const BuildingTemplate* find_by_id(std::string_view id) const noexcept;
  [[nodiscard]] const BuildingTemplate* get_random_by_tier(std::string_view tier, uint32_t seed) const noexcept;
  [[nodiscard]] const BuildingTemplate* get_random_by_category(std::string_view category, uint32_t seed) const noexcept;

  void add_template(BuildingTemplate tmpl) {
    templates.push_back(std::move(tmpl));
  }

  [[nodiscard]] size_t size() const noexcept {
    return templates.size();
  }
};

// Global default catalog accessor
PrefabCatalog& default_catalog();

} // namespace Architecture
