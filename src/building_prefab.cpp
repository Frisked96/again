#include "building_prefab.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <sstream>

namespace Architecture {

namespace {

// Clean, streamlined embedded fallback templates
constexpr std::string_view EMBEDDED_DEFAULT_BUILDINGS_JSON = R"({
  "templates": [
    {
      "id": "peasant_hovel",
      "name": "Peasant Hovel",
      "category": "residential",
      "layout": [
        "######",
        "#....#",
        "#....#",
        "#....#",
        "#....#",
        "##+###"
      ]
    },
    {
      "id": "village_cottage",
      "name": "Village Cottage",
      "category": "residential",
      "layout": [
        "########",
        "#......#",
        "#......#",
        "#...####",
        "#...+..#",
        "#......#",
        "#......#",
        "###+####"
      ]
    },
    {
      "id": "blacksmith_smithy",
      "name": "Village Smithy",
      "category": "commercial",
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
      ]
    },
    {
      "id": "coaching_inn",
      "name": "The Wayfarer's Inn",
      "category": "commercial",
      "settlement_tiers": ["town", "city", "village"],
      "biomes": ["lowland_meadow", "deciduous_weald", "dry_steppe"],
      "wall_type": "WoodWall",
      "floor_type": "WoodFloor",
      "floors": [
        {
          "level": -1,
          "name": "Wine & Ale Cellar",
          "wall_type": "StoneWall",
          "floor_type": "StoneFloor",
          "layout": [
            "##############",
            "#.....#......#",
            "#.=...#...<..#",
            "#.=...#......#",
            "###+####+#####",
            "#............#",
            "#..##....##..#",
            "#..##....##..#",
            "#....#+##....#",
            "#....#..#....#",
            "#....#..#....#",
            "##############"
          ]
        },
        {
          "level": 0,
          "name": "Tavern Common Room & Taproom",
          "layout": [
            "##############",
            "#......#.....#",
            "#.<....#..>..#",
            "#..==..#.....#",
            "#......###+###",
            "#......#.....#",
            "#...+..#.....#",
            "#......###+###",
            "#......#.....#",
            "#......#.....#",
            "#......+.....#",
            "######+#######"
          ]
        },
        {
          "level": 1,
          "name": "Guest Bedchambers & Innkeeper Suite",
          "layout": [
            "##############",
            "#....#...#...#",
            "#.>..+...#...#",
            "#....#...#...#",
            "###+###+###+##",
            "#............#",
            "#............#",
            "##+###+###+###",
            "#...#...#....#",
            "#...#...+....#",
            "#...#...#....#",
            "##############"
          ]
        }
      ]
    },
    {
      "id": "village_chapel",
      "name": "Sanctuary Chapel",
      "category": "religious",
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
      ]
    },
    {
      "id": "guard_outpost",
      "name": "Watchtower Post",
      "category": "military",
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
      ]
    },
    {
      "id": "proc_tavern",
      "name": "Procedural Tavern",
      "category": "commercial",
      "archetype": "tavern",
      "width": 14,
      "height": 12,
      "floors": 2
    },
    {
      "id": "proc_watchtower",
      "name": "Procedural Watchtower",
      "category": "military",
      "archetype": "watchtower",
      "width": 6,
      "height": 6,
      "floors": 2
    }
  ]
})";

Structure::ID parse_structure_type(std::string_view name) noexcept {
  if (name == "StoneWall") return Structure::ID::StoneWall;
  if (name == "WoodWall") return Structure::ID::WoodWall;
  if (name == "Palisade") return Structure::ID::Palisade;
  if (name == "StoneFloor") return Structure::ID::StoneFloor;
  if (name == "WoodFloor") return Structure::ID::WoodFloor;
  if (name == "DoorClosed") return Structure::ID::DoorClosed;
  if (name == "DoorOpen") return Structure::ID::DoorOpen;
  if (name == "Window") return Structure::ID::Window;
  if (name == "StairsDown") return Structure::ID::StairsDown;
  if (name == "StairsUp") return Structure::ID::StairsUp;
  if (name == "Ladder") return Structure::ID::Ladder;
  if (name == "Anvil") return Structure::ID::Anvil;
  if (name == "Counter") return Structure::ID::Counter;
  return Structure::ID::None;
}

// Token parser for reading JSON
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

// ---------------------------------------------------------------------------
// Room Detection via Flood Fill
// ---------------------------------------------------------------------------
std::vector<RoomDescriptor> RoomDetector::detect_rooms(
    const std::vector<std::string> &layout,
    const std::vector<std::string> &suggested_names)
{
  std::vector<RoomDescriptor> detected;
  if (layout.empty()) return detected;

  int h = static_cast<int>(layout.size());
  int w = static_cast<int>(layout[0].size());
  std::vector<std::vector<bool>> visited(h, std::vector<bool>(w, false));

  auto is_interior_room_glyph = [](char c) noexcept {
    return c == '.' || c == '&' || c == '=' || c == '<' || c == '>' || c == 'H';
  };

  int room_counter = 0;
  for (int y = 0; y < h; ++y) {
    for (int x = 0; x < w; ++x) {
      if (!visited[y][x] && is_interior_room_glyph(layout[y][x])) {
        int min_x = x, max_x = x, min_y = y, max_y = y;
        bool has_anvil = false;
        bool has_counter = false;
        bool has_stairs = false;
        int tile_count = 0;

        std::queue<std::pair<int, int>> q;
        q.push({x, y});
        visited[y][x] = true;

        while (!q.empty()) {
          auto [cx, cy] = q.front();
          q.pop();
          ++tile_count;

          char g = layout[cy][cx];
          if (g == '&') has_anvil = true;
          if (g == '=') has_counter = true;
          if (g == '<' || g == '>') has_stairs = true;

          min_x = std::min(min_x, cx);
          max_x = std::max(max_x, cx);
          min_y = std::min(min_y, cy);
          max_y = std::max(max_y, cy);

          static const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
          for (const auto &d : dirs) {
            int nx = cx + d[0];
            int ny = cy + d[1];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && !visited[ny][nx]) {
              if (is_interior_room_glyph(layout[ny][nx])) {
                visited[ny][nx] = true;
                q.push({nx, ny});
              }
            }
          }
        }

        std::string name;
        if (room_counter < static_cast<int>(suggested_names.size()) && !suggested_names[room_counter].empty()) {
          name = suggested_names[room_counter];
        } else {
          if (has_anvil) name = "Forge & Anvil Court";
          else if (has_counter) name = "Tavern Taproom";
          else if (has_stairs) name = "Stairwell & Hall";
          else if (tile_count <= 8) name = "Pantry & Cold Stores";
          else if (tile_count >= 25) name = "Common Great Hall";
          else if (room_counter == 0) name = "Main Chamber";
          else name = "Bedchamber " + std::to_string(room_counter);
        }

        detected.push_back(RoomDescriptor{std::move(name), min_x, min_y, max_x, max_y});
        ++room_counter;
      }
    }
  }

  return detected;
}

// ---------------------------------------------------------------------------
// BuildingTemplate Helpers
// ---------------------------------------------------------------------------
void BuildingTemplate::infer_missing_data() {
  if (width <= 0 || height <= 0) {
    if (!floors.empty()) {
      for (const auto &f : floors) {
        if (!f.layout.empty()) {
          height = static_cast<int>(f.layout.size());
          width = static_cast<int>(f.layout[0].size());
          break;
        }
      }
    } else if (!layout.empty()) {
      height = static_cast<int>(layout.size());
      width = static_cast<int>(layout[0].size());
    }
  }

  // Derive title-cased name from id if empty
  if (name.empty() && !id.empty()) {
    name = id;
    bool cap_next = true;
    for (char &c : name) {
      if (c == '_') {
        c = ' ';
        cap_next = true;
      } else if (cap_next) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        cap_next = false;
      }
    }
  }

  // Default tiers
  if (settlement_tiers.empty()) {
    settlement_tiers = {"hamlet", "village", "town", "city"};
  }

  // Ensure default floor 0 is populated from top-level layout or vice-versa
  if (floors.empty() && !layout.empty()) {
    floors.push_back(BuildingFloor{
      0,
      "Ground Floor",
      wall_type,
      floor_type,
      layout,
      rooms
    });
  } else if (layout.empty() && !floors.empty()) {
    for (const auto &f : floors) {
      if (f.level == 0) {
        layout = f.layout;
        rooms = f.rooms;
        wall_type = f.wall_type;
        floor_type = f.floor_type;
        break;
      }
    }
    if (layout.empty()) {
      layout = floors[0].layout;
      rooms = floors[0].rooms;
      wall_type = floors[0].wall_type;
      floor_type = floors[0].floor_type;
    }
  }

  // Auto-detect rooms if rooms array is empty
  if (rooms.empty() && !layout.empty()) {
    rooms = RoomDetector::detect_rooms(layout);
  }
  for (auto &f : floors) {
    if (f.rooms.empty() && !f.layout.empty()) {
      f.rooms = RoomDetector::detect_rooms(f.layout);
    }
  }
}

// ---------------------------------------------------------------------------
// Procedural Building Generator
// ---------------------------------------------------------------------------
BuildingTemplate BuildingGenerator::generate(const ProceduralBuildingConfig &config, uint32_t seed) {
  BuildingTemplate tmpl;
  std::string arch = config.archetype.empty() ? "cottage" : config.archetype;
  tmpl.id = config.id.empty() ? ("proc_" + arch + "_" + std::to_string(seed)) : config.id;
  tmpl.name = config.name;

  // 1. Resolve dimensions
  int W = config.width;
  int H = config.height;
  if (W <= 0 || H <= 0) {
    if (arch == "hovel") { W = 6; H = 6; }
    else if (arch == "cottage") { W = 8; H = 8; }
    else if (arch == "townhouse" || arch == "house") { W = 10; H = 8; }
    else if (arch == "manor") { W = 14; H = 12; }
    else if (arch == "smithy") { W = 11; H = 9; }
    else if (arch == "tavern" || arch == "inn") { W = 14; H = 12; }
    else if (arch == "shop") { W = 9; H = 8; }
    else if (arch == "watchtower") { W = 6; H = 6; }
    else if (arch == "guardpost" || arch == "barracks") { W = 10; H = 9; }
    else if (arch == "chapel" || arch == "shrine") { W = 11; H = 13; }
    else if (arch == "warehouse") { W = 12; H = 8; }
    else { W = 8; H = 8; }
  }
  W = std::max(W, 5);
  H = std::max(H, 5);
  tmpl.width = W;
  tmpl.height = H;

  // 2. Resolve Category
  if (!config.category.empty()) {
    tmpl.category = config.category;
  } else {
    if (arch == "smithy" || arch == "tavern" || arch == "inn" || arch == "shop") tmpl.category = "commercial";
    else if (arch == "watchtower" || arch == "guardpost" || arch == "barracks") tmpl.category = "military";
    else if (arch == "chapel" || arch == "shrine") tmpl.category = "religious";
    else if (arch == "warehouse" || arch == "workshop") tmpl.category = "industrial";
    else tmpl.category = "residential";
  }

  // 3. Resolve Materials
  if (config.wall_type != Structure::ID::None) {
    tmpl.wall_type = config.wall_type;
  } else {
    if (arch == "smithy" || arch == "watchtower" || arch == "guardpost" ||
        arch == "barracks" || arch == "chapel" || arch == "shrine") {
      tmpl.wall_type = Structure::ID::StoneWall;
    } else {
      tmpl.wall_type = Structure::ID::WoodWall;
    }
  }

  if (config.floor_type != Structure::ID::None) {
    tmpl.floor_type = config.floor_type;
  } else {
    tmpl.floor_type = (tmpl.wall_type == Structure::ID::StoneWall)
                      ? Structure::ID::StoneFloor
                      : Structure::ID::WoodFloor;
  }

  tmpl.settlement_tiers = config.settlement_tiers;
  tmpl.biomes = config.biomes;

  // 4. Resolve Floors count
  int num_floors = config.num_floors;
  if (num_floors <= 0) {
    if (arch == "watchtower" || arch == "tavern" || arch == "manor") num_floors = 2;
    else num_floors = 1;
  }
  num_floors = std::clamp(num_floors, 1, 3);

  // 5. Generate Ground Floor (Level 0)
  std::vector<std::string> layout0(H, std::string(W, '#'));
  for (int y = 1; y < H - 1; ++y) {
    for (int x = 1; x < W - 1; ++x) {
      layout0[y][x] = '.';
    }
  }

  // Exterior Door on south wall
  int door_x = W / 2;
  layout0[H - 1][door_x] = '+';
  layout0[H - 2][door_x] = '.'; // Guaranteed clear entry path

  // Windows along north and south exterior walls
  if (W >= 7) {
    layout0[0][2] = '"';
    layout0[0][W - 3] = '"';
    if (door_x != 2) layout0[H - 1][2] = '"';
    if (door_x != W - 3) layout0[H - 1][W - 3] = '"';
  }

  // Stairs positions (avoiding door_x column)
  int stair_up_x = (door_x <= 2 ? W - 3 : 2);
  int stair_up_y = 2;
  int stair_down_x = (stair_up_x == 2 ? W - 3 : 2);
  int stair_down_y = 2;

  if (num_floors >= 2) {
    layout0[stair_up_y][stair_up_x] = '<'; // StairsUp to level 1
  }
  if (num_floors >= 3) {
    if (stair_down_x == stair_up_x) {
      stair_down_y = H - 3;
    }
    layout0[stair_down_y][stair_down_x] = '>'; // StairsDown to level -1 (cellar)
  }

  // Archetype fixtures and partitioning for Ground Floor
  if (arch == "smithy") {
    int div_x = std::max(2, W - 4);
    if (div_x == door_x) div_x = door_x + 1;
    for (int y = 1; y < H - 1; ++y) {
      if (layout0[y][div_x] == '.') layout0[y][div_x] = '#';
    }
    layout0[H / 2][div_x] = '+'; // Interior connecting door
    layout0[H - 2][door_x] = '.'; // Entry path clear
    int anvil_x = (stair_up_x == 2 ? 3 : 2);
    if (anvil_x != door_x && anvil_x < div_x) {
      layout0[3][anvil_x] = '&'; // Anvil
    }
  } else if (arch == "tavern" || arch == "inn") {
    // Counter in taproom
    int counter_x = (door_x > 3 ? 3 : W - 4);
    layout0[3][counter_x] = '=';
    layout0[3][counter_x + 1] = '=';
    // Dividing wall for pantry / backroom
    if (W >= 10 && H >= 8) {
      int div_x = W - 4;
      if (div_x == door_x) div_x = door_x - 2;
      for (int y = 1; y < H - 1; ++y) {
        if (layout0[y][div_x] == '.') layout0[y][div_x] = '#';
      }
      layout0[H / 2][div_x] = '+';
      layout0[H - 2][door_x] = '.';
    }
  } else if (arch == "chapel" || arch == "shrine") {
    // Sanctuary pillar / altar
    if (H >= 7 && W >= 7) {
      layout0[3][W / 2] = '#';
    }
  } else if (arch == "shop") {
    // Counter
    int cx = (door_x <= 3 ? W - 4 : 2);
    layout0[3][cx] = '=';
    layout0[3][cx + 1] = '=';
  } else {
    // Residential / military / general partitioning:
    // Add interior room dividing wall if building is large enough
    if (W >= 8 && H >= 8) {
      int div_x = (door_x >= 4) ? (door_x - 2) : (door_x + 2);
      for (int y = 1; y < H - 1; ++y) {
        if (layout0[y][div_x] == '.') layout0[y][div_x] = '#';
      }
      layout0[H / 2][div_x] = '+'; // Guaranteed connecting door
      layout0[H - 2][door_x] = '.'; // Entry path clear
    }
  }

  // Add Ground Floor
  BuildingFloor ground_floor;
  ground_floor.level = 0;
  ground_floor.name = "Ground Floor";
  ground_floor.wall_type = tmpl.wall_type;
  ground_floor.floor_type = tmpl.floor_type;
  ground_floor.layout = layout0;
  tmpl.floors.push_back(std::move(ground_floor));

  // 6. Upper Floor (Level 1) if num_floors >= 2
  if (num_floors >= 2) {
    std::vector<std::string> layout1(H, std::string(W, '#'));
    for (int y = 1; y < H - 1; ++y) {
      for (int x = 1; x < W - 1; ++x) {
        layout1[y][x] = '.';
      }
    }
    // Matching StairsDown at exact same coordinates as level 0 StairsUp!
    layout1[stair_up_y][stair_up_x] = '>';

    // Windows
    if (W >= 7) {
      layout1[0][2] = '"';
      layout1[0][W - 3] = '"';
      layout1[H - 1][2] = '"';
      layout1[H - 1][W - 3] = '"';
    }

    // Upper hallway / bedchamber divider if large enough
    if (W >= 10 && H >= 8) {
      int div_y = H / 2;
      for (int x = 1; x < W - 1; ++x) {
        if (layout1[div_y][x] == '.') layout1[div_y][x] = '#';
      }
      layout1[div_y][stair_up_x] = '+'; // Door connects rooms
    }

    BuildingFloor upper_floor;
    upper_floor.level = 1;
    upper_floor.name = "Upper Floor";
    upper_floor.wall_type = tmpl.wall_type;
    upper_floor.floor_type = tmpl.floor_type;
    upper_floor.layout = layout1;
    tmpl.floors.push_back(std::move(upper_floor));
  }

  // 7. Cellar Floor (Level -1) if num_floors >= 3
  if (num_floors >= 3) {
    std::vector<std::string> layout_neg1(H, std::string(W, '#'));
    for (int y = 1; y < H - 1; ++y) {
      for (int x = 1; x < W - 1; ++x) {
        layout_neg1[y][x] = '.';
      }
    }
    // Matching StairsUp at exact same coordinates as level 0 StairsDown!
    layout_neg1[stair_down_y][stair_down_x] = '<';

    // Cellar vault wall
    if (W >= 10 && H >= 8) {
      int div_x = W / 2;
      for (int y = 1; y < H - 1; ++y) {
        if (layout_neg1[y][div_x] == '.') layout_neg1[y][div_x] = '#';
      }
      layout_neg1[H / 2][div_x] = '+'; // Vault door
    }

    BuildingFloor cellar_floor;
    cellar_floor.level = -1;
    cellar_floor.name = "Cellar & Vault";
    cellar_floor.wall_type = Structure::ID::StoneWall;
    cellar_floor.floor_type = Structure::ID::StoneFloor;
    cellar_floor.layout = layout_neg1;
    tmpl.floors.push_back(std::move(cellar_floor));
  }

  // 8. Auto-infer and auto-detect rooms
  tmpl.infer_missing_data();
  return tmpl;
}

BuildingTemplate BuildingGenerator::generate_random(std::string_view category, std::string_view tier, uint32_t seed) {
  ProceduralBuildingConfig cfg;
  cfg.category = category.empty() ? "residential" : std::string(category);
  cfg.settlement_tiers = {std::string(tier.empty() ? "village" : tier)};

  if (cfg.category == "commercial") {
    cfg.archetype = (seed % 2 == 0) ? "smithy" : "tavern";
  } else if (cfg.category == "military") {
    cfg.archetype = (seed % 2 == 0) ? "watchtower" : "guardpost";
  } else if (cfg.category == "religious") {
    cfg.archetype = "chapel";
  } else if (cfg.category == "industrial") {
    cfg.archetype = "warehouse";
  } else {
    // residential
    uint32_t r = seed % 3;
    if (r == 0) cfg.archetype = "hovel";
    else if (r == 1) cfg.archetype = "cottage";
    else cfg.archetype = "townhouse";
  }

  cfg.id = "proc_" + cfg.archetype + "_" + std::to_string(seed);
  return generate(cfg, seed);
}

// ---------------------------------------------------------------------------
// PrefabCatalog Implementation
// ---------------------------------------------------------------------------
bool PrefabCatalog::load(const std::string &filepath) {
  templates.clear();
  std::ifstream f(filepath);
  if (f.is_open()) {
    std::stringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();
    if (load_from_string(content)) {
      return true;
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

  uint32_t proc_seed = 100;

  // Process all template objects
  while (reader.has_more()) {
    size_t template_start = json_str.find('{', reader.get_pos());
    if (template_start == std::string_view::npos) {
      break;
    }
    reader.set_pos(template_start + 1);

    // Find matching brace for template object
    int brace_depth = 1;
    size_t search_pos = template_start + 1;
    size_t template_end = template_start;
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

    // Check if this template is a procedural generator archetype
    std::string archetype;
    if (tmpl_reader.find_key("archetype")) archetype = tmpl_reader.read_string();
    tmpl_reader.set_pos(0);
    if (archetype.empty() && tmpl_reader.find_key("generator")) archetype = tmpl_reader.read_string();
    tmpl_reader.set_pos(0);

    if (!archetype.empty()) {
      ProceduralBuildingConfig cfg;
      cfg.archetype = archetype;
      if (tmpl_reader.find_key("id")) cfg.id = tmpl_reader.read_string();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("name")) cfg.name = tmpl_reader.read_string();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("category")) cfg.category = tmpl_reader.read_string();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("width")) cfg.width = tmpl_reader.read_int();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("height")) cfg.height = tmpl_reader.read_int();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("floors")) cfg.num_floors = tmpl_reader.read_int();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("num_floors")) cfg.num_floors = tmpl_reader.read_int();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("wall_type")) cfg.wall_type = parse_structure_type(tmpl_reader.read_string());
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("floor_type")) cfg.floor_type = parse_structure_type(tmpl_reader.read_string());
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("settlement_tiers")) cfg.settlement_tiers = tmpl_reader.read_string_array();
      tmpl_reader.set_pos(0);
      if (tmpl_reader.find_key("biomes")) cfg.biomes = tmpl_reader.read_string_array();

      BuildingTemplate gen_tmpl = BuildingGenerator::generate(cfg, proc_seed++);
      if (gen_tmpl.is_valid()) {
        templates.push_back(std::move(gen_tmpl));
      }
      reader.set_pos(template_end + 1);
      continue;
    }

    // Otherwise, parse streamlined handcrafted template
    BuildingTemplate tmpl;
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

    Structure::ID wall_t = Structure::ID::None;
    if (tmpl_reader.find_key("wall_type")) wall_t = parse_structure_type(tmpl_reader.read_string());
    tmpl.wall_type = (wall_t != Structure::ID::None) ? wall_t : Structure::ID::WoodWall;
    tmpl_reader.set_pos(0);

    Structure::ID floor_t = Structure::ID::None;
    if (tmpl_reader.find_key("floor_type")) floor_t = parse_structure_type(tmpl_reader.read_string());
    tmpl.floor_type = (floor_t != Structure::ID::None) ? floor_t : Structure::ID::WoodFloor;
    tmpl_reader.set_pos(0);

    if (tmpl_reader.find_key("layout")) tmpl.layout = tmpl_reader.read_string_array();

    // Parse floors array if present
    tmpl_reader.set_pos(0);
    if (tmpl_reader.find_key("floors")) {
      size_t floors_array_pos = tmpl_reader.get_pos();
      size_t floor_obj_pos = tmpl_chunk.find('{', floors_array_pos);
      while (floor_obj_pos != std::string_view::npos && floor_obj_pos < tmpl_chunk.size()) {
        int f_depth = 1;
        size_t f_search = floor_obj_pos + 1;
        size_t floor_obj_end = floor_obj_pos;
        while (f_search < tmpl_chunk.size() && f_depth > 0) {
          if (tmpl_chunk[f_search] == '{') ++f_depth;
          else if (tmpl_chunk[f_search] == '}') --f_depth;
          if (f_depth == 0) {
            floor_obj_end = f_search;
            break;
          }
          ++f_search;
        }

        std::string_view floor_chunk = tmpl_chunk.substr(floor_obj_pos, floor_obj_end - floor_obj_pos + 1);
        SimpleJsonReader floor_reader(floor_chunk);
        BuildingFloor bf;
        if (floor_reader.find_key("level")) bf.level = floor_reader.read_int();
        floor_reader.set_pos(0);
        if (floor_reader.find_key("name")) bf.name = floor_reader.read_string();
        floor_reader.set_pos(0);

        Structure::ID f_wall_t = Structure::ID::None;
        if (floor_reader.find_key("wall_type")) f_wall_t = parse_structure_type(floor_reader.read_string());
        bf.wall_type = (f_wall_t != Structure::ID::None) ? f_wall_t : tmpl.wall_type;
        floor_reader.set_pos(0);

        Structure::ID f_floor_t = Structure::ID::None;
        if (floor_reader.find_key("floor_type")) f_floor_t = parse_structure_type(floor_reader.read_string());
        bf.floor_type = (f_floor_t != Structure::ID::None) ? f_floor_t : tmpl.floor_type;
        floor_reader.set_pos(0);

        if (floor_reader.find_key("layout")) bf.layout = floor_reader.read_string_array();

        if (!bf.layout.empty()) {
          tmpl.floors.push_back(std::move(bf));
        }

        floor_obj_pos = tmpl_chunk.find('{', floor_obj_end + 1);
      }
    }

    tmpl.infer_missing_data();

    if (tmpl.is_valid()) {
      templates.push_back(std::move(tmpl));
    }

    reader.set_pos(template_end + 1);
  }

  return !templates.empty();
}

BuildingTemplate PrefabCatalog::generate_and_add(const ProceduralBuildingConfig &config, uint32_t seed) {
  BuildingTemplate tmpl = BuildingGenerator::generate(config, seed);
  templates.push_back(tmpl);
  return tmpl;
}

const BuildingTemplate* PrefabCatalog::get_random_or_generate(std::string_view category, std::string_view tier, uint32_t seed) {
  const BuildingTemplate* found = nullptr;
  if (!category.empty()) {
    found = get_random_by_category(category, seed);
  } else if (!tier.empty()) {
    found = get_random_by_tier(tier, seed);
  }
  if (found) return found;

  // Generate on the fly and store
  BuildingTemplate gen = BuildingGenerator::generate_random(category, tier, seed);
  templates.push_back(std::move(gen));
  return &templates.back();
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
    return nullptr;
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
    return nullptr;
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
