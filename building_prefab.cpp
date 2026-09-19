#include "building_prefab.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

namespace Architecture {

namespace {

// Embedded default JSON fallback ensuring zero-dependency standalone execution
constexpr std::string_view EMBEDDED_DEFAULT_BUILDINGS_JSON = R"({
  "templates": [
    {
      "id": "peasant_hovel",
      "name": "Peasant Hovel",
      "category": "residential",
      "settlement_tiers": ["hamlet", "village"],
      "biomes": ["lowland_meadow", "deciduous_weald", "peat_fen", "dry_steppe"],
      "width": 6,
      "height": 6,
      "wall_type": "WoodWall",
      "floor_type": "WoodFloor",
      "layout": [
        "######",
        "#....#",
        "#....#",
        "#....#",
        "#....#",
        "##+###"
      ],
      "rooms": [
        {
          "name": "Hovel Living Space",
          "x1": 1, "y1": 1, "x2": 4, "y2": 4
        }
      ]
    },
    {
      "id": "village_cottage",
      "name": "Village Cottage",
      "category": "residential",
      "settlement_tiers": ["village", "town"],
      "biomes": ["lowland_meadow", "deciduous_weald", "dry_steppe"],
      "width": 8,
      "height": 8,
      "wall_type": "WoodWall",
      "floor_type": "WoodFloor",
      "layout": [
        "########",
        "#......#",
        "#......#",
        "#...####",
        "#...+..#",
        "#......#",
        "#......#",
        "###+####"
      ],
      "rooms": [
        {
          "name": "Living Room & Hearth",
          "x1": 1, "y1": 1, "x2": 6, "y2": 3
        },
        {
          "name": "Bedroom",
          "x1": 4, "y1": 4, "x2": 6, "y2": 6
        }
      ]
    },
    {
      "id": "blacksmith_smithy",
      "name": "Village Smithy",
      "category": "commercial",
      "settlement_tiers": ["village", "town", "city"],
      "biomes": ["lowland_meadow", "deciduous_weald", "dry_steppe", "highland_peaks"],
      "width": 11,
      "height": 9,
      "wall_type": "StoneWall",
      "floor_type": "StoneFloor",
      "layout": [
        "###########",
        "#.....#...#",
        "#..&..#...#",
        "#.....#...#",
        "#.....#...+",
        "#.....#...#",
        "#.....#...#",
        "#.....#...#",
        "#####+#####"
      ],
      "rooms": [
        {
          "name": "Forge & Anvil Court",
          "x1": 1, "y1": 1, "x2": 5, "y2": 7
        },
        {
          "name": "Smith's Quarters",
          "x1": 7, "y1": 1, "x2": 9, "y2": 7
        }
      ]
    },
    {
      "id": "coaching_inn",
      "name": "The Wayfarer's Inn",
      "category": "commercial",
      "settlement_tiers": ["town", "city", "village"],
      "biomes": ["lowland_meadow", "deciduous_weald", "dry_steppe"],
      "width": 14,
      "height": 12,
      "wall_type": "WoodWall",
      "floor_type": "WoodFloor",
      "layout": [
        "##############",
        "#......#.....#",
        "#......#.....#",
        "#..==..#..>..#",
        "#......###+###",
        "#......#.....#",
        "#......+.....#",
        "#......###+###",
        "#......#.....#",
        "#......+.....#",
        "#......#.....#",
        "######+#######"
      ],
      "rooms": [
        {
          "name": "Tavern Taproom",
          "x1": 1, "y1": 1, "x2": 6, "y2": 10
        },
        {
          "name": "Cellar Storage",
          "x1": 8, "y1": 1, "x2": 12, "y2": 3
        },
        {
          "name": "Guest Room North",
          "x1": 8, "y1": 5, "x2": 12, "y2": 6
        },
        {
          "name": "Guest Room South",
          "x1": 8, "y1": 8, "x2": 12, "y2": 10
        }
      ]
    },
    {
      "id": "village_chapel",
      "name": "Sanctuary Chapel",
      "category": "religious",
      "settlement_tiers": ["village", "town", "city"],
      "biomes": ["lowland_meadow", "deciduous_weald", "peat_fen", "highland_peaks"],
      "width": 11,
      "height": 13,
      "wall_type": "StoneWall",
      "floor_type": "StoneFloor",
      "layout": [
        "###########",
        "#....>....#",
        "#.........#",
        "#....#....#",
        "\".........\"",
        "#.........#",
        "#.........#",
        "#.........#",
        "\".........\"",
        "#.........#",
        "#.........#",
        "#.........#",
        "#####+#####"
      ],
      "rooms": [
        {
          "name": "Altar Sanctuary",
          "x1": 1, "y1": 1, "x2": 9, "y2": 3
        },
        {
          "name": "Nave",
          "x1": 1, "y1": 4, "x2": 9, "y2": 11
        }
      ]
    },
    {
      "id": "guard_outpost",
      "name": "Watchtower Post",
      "category": "military",
      "settlement_tiers": ["hamlet", "village", "town", "city"],
      "biomes": ["lowland_meadow", "deciduous_weald", "frost_tundra", "arid_waste"],
      "width": 8,
      "height": 8,
      "wall_type": "StoneWall",
      "floor_type": "StoneFloor",
      "layout": [
        "########",
        "\"......\"",
        "#......#",
        "#......#",
        "#..>...#",
        "#......#",
        "\"......\"",
        "###+####"
      ],
      "rooms": [
        {
          "name": "Guardpost & Armory",
          "x1": 1, "y1": 1, "x2": 6, "y2": 6
        }
      ]
    }
  ]
})";

Tile::ID parse_tile_type(std::string_view name) noexcept {
  if (name == "StoneWall") return Tile::ID::StoneWall;
  if (name == "WoodWall") return Tile::ID::WoodWall;
  if (name == "StoneFloor") return Tile::ID::StoneFloor;
  if (name == "WoodFloor") return Tile::ID::WoodFloor;
  if (name == "Cobblestone") return Tile::ID::Cobblestone;
  if (name == "DirtRoad") return Tile::ID::DirtRoad;
  return Tile::ID::WoodFloor;
}

// Minimal robust JSON token parser for template files
class SimpleJsonReader {
private:
  std::string_view src;
  size_t pos{0};

  void skip_whitespace() {
    while (pos < src.size() && (std::isspace(static_cast<unsigned char>(src[pos])) || src[pos] == ',' || src[pos] == ':')) {
      ++pos;
    }
  }

public:
  explicit SimpleJsonReader(std::string_view s) : src(s) {}

  bool has_more() {
    skip_whitespace();
    return pos < src.size();
  }

  std::string read_string() {
    skip_whitespace();
    if (pos >= src.size() || src[pos] != '"') {
      return "";
    }
    ++pos; // skip open quote
    std::string result;
    while (pos < src.size() && src[pos] != '"') {
      if (src[pos] == '\\' && pos + 1 < src.size()) {
        ++pos;
        if (src[pos] == '"') result.push_back('"');
        else if (src[pos] == '\\') result.push_back('\\');
        else if (src[pos] == 'n') result.push_back('\n');
        else if (src[pos] == 't') result.push_back('\t');
        else result.push_back(src[pos]);
      } else {
        result.push_back(src[pos]);
      }
      ++pos;
    }
    if (pos < src.size()) ++pos; // skip close quote
    return result;
  }

  int read_int() {
    skip_whitespace();
    bool neg = false;
    if (pos < src.size() && src[pos] == '-') {
      neg = true;
      ++pos;
    }
    int val = 0;
    while (pos < src.size() && std::isdigit(static_cast<unsigned char>(src[pos]))) {
      val = val * 10 + (src[pos] - '0');
      ++pos;
    }
    return neg ? -val : val;
  }

  std::vector<std::string> read_string_array() {
    skip_whitespace();
    std::vector<std::string> arr;
    if (pos >= src.size() || src[pos] != '[') {
      return arr;
    }
    ++pos; // skip '['
    while (pos < src.size()) {
      skip_whitespace();
      if (pos < src.size() && src[pos] == ']') {
        ++pos;
        break;
      }
      if (pos < src.size() && src[pos] == '"') {
        arr.push_back(read_string());
      } else {
        ++pos;
      }
    }
    return arr;
  }

  bool find_key(std::string_view key) {
    std::string needle = "\"" + std::string(key) + "\"";
    size_t found = src.find(needle, pos);
    if (found != std::string_view::npos) {
      pos = found + needle.size();
      skip_whitespace();
      return true;
    }
    return false;
  }

  size_t get_pos() const noexcept { return pos; }
  void set_pos(size_t p) noexcept { pos = p; }
};

} // namespace

bool PrefabCatalog::load(const std::string &filepath) {
  templates.clear();

  std::vector<std::string> candidates;
  if (!filepath.empty()) {
    candidates.push_back(filepath);
  }
  candidates.push_back("data/prefabs/buildings.json");
  candidates.push_back("../data/prefabs/buildings.json");

  // Attempt resolving relative to current executable
  std::error_code ec;
  auto exe_symlink = std::filesystem::read_symlink("/proc/self/exe", ec);
  if (!ec) {
    auto exe_dir = exe_symlink.parent_path();
    candidates.push_back((exe_dir / "data/prefabs/buildings.json").string());
    candidates.push_back((exe_dir / "../data/prefabs/buildings.json").string());
  }

  for (const auto &p : candidates) {
    std::ifstream f(p);
    if (f.is_open()) {
      std::stringstream ss;
      ss << f.rdbuf();
      std::string content = ss.str();
      if (load_from_string(content)) {
        return true;
      }
    }
  }

  // Fallback to embedded default JSON
  return load_from_string(EMBEDDED_DEFAULT_BUILDINGS_JSON);
}

bool PrefabCatalog::load_from_string(std::string_view json_str) {
  templates.clear();
  SimpleJsonReader reader(json_str);

  // Locate "templates" array
  if (!reader.find_key("templates")) {
    return false;
  }

  // Find array open '['
  while (reader.has_more()) {
    // Find next template object start '{'
    size_t template_start = json_str.find('{', reader.get_pos());
    if (template_start == std::string_view::npos) {
      break;
    }
    reader.set_pos(template_start + 1);

    BuildingTemplate tmpl;

    // Scan properties within this template object until closing '}'
    size_t template_end = json_str.find('}', template_start);
    // Note: there may be inner objects (rooms), so find matching brace
    int brace_depth = 1;
    size_t search_pos = template_start + 1;
    while (search_pos < json_str.size() && brace_depth > 0) {
      if (json_str[search_pos] == '{') ++brace_depth;
      else if (json_str[search_pos] == '}') --brace_depth;
      if (brace_depth == 0) {
        template_end = search_pos;
        break;
      }
      ++search_pos;
    }

    std::string_view tmpl_chunk = json_str.substr(template_start, template_end - template_start + 1);
    SimpleJsonReader tmpl_reader(tmpl_chunk);

    if (tmpl_reader.find_key("id")) tmpl.id = tmpl_reader.read_string();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("name")) tmpl.name = tmpl_reader.read_string();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("category")) tmpl.category = tmpl_reader.read_string();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("settlement_tiers")) tmpl.settlement_tiers = tmpl_reader.read_string_array();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("biomes")) tmpl.biomes = tmpl_reader.read_string_array();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("width")) tmpl.width = tmpl_reader.read_int();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("height")) tmpl.height = tmpl_reader.read_int();
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("wall_type")) tmpl.wall_type = parse_tile_type(tmpl_reader.read_string());
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("floor_type")) tmpl.floor_type = parse_tile_type(tmpl_reader.read_string());
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("layout")) tmpl.layout = tmpl_reader.read_string_array();

    // Parse rooms array if present
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("rooms")) {
      size_t rooms_array_pos = tmpl_reader.get_pos();
      size_t room_obj_pos = tmpl_chunk.find('{', rooms_array_pos);
      while (room_obj_pos != std::string_view::npos && room_obj_pos < tmpl_chunk.size()) {
        size_t room_obj_end = tmpl_chunk.find('}', room_obj_pos);
        if (room_obj_end == std::string_view::npos) break;

        std::string_view room_chunk = tmpl_chunk.substr(room_obj_pos, room_obj_end - room_obj_pos + 1);
        SimpleJsonReader room_reader(room_chunk);
        RoomDescriptor rd;
        if (room_reader.find_key("name")) rd.name = room_reader.read_string();
        room_reader.set_pos(0);
        if (room_reader.find_key("x1")) rd.x1 = room_reader.read_int();
        room_reader.set_pos(0);
        if (room_reader.find_key("y1")) rd.y1 = room_reader.read_int();
        room_reader.set_pos(0);
        if (room_reader.find_key("x2")) rd.x2 = room_reader.read_int();
        room_reader.set_pos(0);
        if (room_reader.find_key("y2")) rd.y2 = room_reader.read_int();

        if (!rd.name.empty()) {
          tmpl.rooms.push_back(std::move(rd));
        }

        room_obj_pos = tmpl_chunk.find('{', room_obj_end + 1);
      }
    }

    if (tmpl.is_valid()) {
      templates.push_back(std::move(tmpl));
    }

    reader.set_pos(template_end + 1);
  }

  return !templates.empty();
}

const BuildingTemplate* PrefabCatalog::find_by_id(std::string_view id) const noexcept {
  for (const auto &t : templates) {
    if (t.id == id) {
      return &t;
    }
  }
  return nullptr;
}

const BuildingTemplate* PrefabCatalog::get_random_by_tier(std::string_view tier, uint32_t seed) const noexcept {
  std::vector<const BuildingTemplate*> matches;
  for (const auto &t : templates) {
    if (t.settlement_tiers.empty()) {
      matches.push_back(&t);
      continue;
    }
    for (const auto &s : t.settlement_tiers) {
      if (s == tier) {
        matches.push_back(&t);
        break;
      }
    }
  }
  if (matches.empty()) {
    return templates.empty() ? nullptr : &templates[seed % templates.size()];
  }
  return matches[seed % matches.size()];
}

const BuildingTemplate* PrefabCatalog::get_random_by_category(std::string_view category, uint32_t seed) const noexcept {
  std::vector<const BuildingTemplate*> matches;
  for (const auto &t : templates) {
    if (t.category == category) {
      matches.push_back(&t);
    }
  }
  if (matches.empty()) {
    return templates.empty() ? nullptr : &templates[seed % templates.size()];
  }
  return matches[seed % matches.size()];
}

PrefabCatalog& default_catalog() {
  static PrefabCatalog catalog;
  static bool initialized = false;
  if (!initialized) {
    catalog.load();
    initialized = true;
  }
  return catalog;
}

} // namespace Architecture
