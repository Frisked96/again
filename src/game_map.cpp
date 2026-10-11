#include "game_map.hpp"
#include "building_prefab.hpp"
#include "vision.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>

namespace GameMap {

Map::Map(int w, int h, Tile::ID default_tile, World::RegionID default_region)
    : width(w), height(h),
      tile_grid(static_cast<size_t>(w), static_cast<size_t>(h), default_tile),
      region_grid(static_cast<size_t>(w), static_cast<size_t>(h), default_region),
      vegetation_grid(static_cast<size_t>(w), static_cast<size_t>(h), Vegetation::Cell{Vegetation::ID::None, 0}),
      surface_structure_grid(static_cast<size_t>(w), static_cast<size_t>(h), Structure::Cell{Structure::ID::None, 0}) {}

void Map::resize(int w, int h, Tile::ID default_tile, World::RegionID default_region) {
  width = w;
  height = h;
  tile_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), default_tile);
  region_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), default_region);
  vegetation_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), Vegetation::Cell{Vegetation::ID::None, 0});
  surface_structure_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), Structure::Cell{Structure::ID::None, 0});
  vertical_structures.clear();
}

void Map::clear(Tile::ID fill_tile, World::RegionID fill_region, Vegetation::ID fill_veg) {
  tile_grid.fill(fill_tile);
  region_grid.fill(fill_region);
  uint8_t yield = (fill_veg == Vegetation::ID::None) ? 0 : Vegetation::getData(fill_veg).maxYield;
  vegetation_grid.fill(Vegetation::Cell{fill_veg, yield});
  surface_structure_grid.fill(Structure::Cell{Structure::ID::None, 0});
  vertical_structures.clear();
}

void Map::set(int x, int y, Tile::ID id) noexcept {
  if (in_bounds(x, y)) {
    tile_grid(x, y) = id;
  }
}

Tile::ID Map::at(int x, int y, int z) const noexcept {
  if (!in_bounds(x, y)) {
    return Tile::ID::Void;
  }
  if (z == 0) {
    return tile_grid(x, y);
  } else if (z > 0) {
    return Tile::ID::OpenAir;
  } else {
    return Tile::ID::SubterraneanRock;
  }
}

void Map::set_structure(int x, int y, Structure::ID id, uint8_t durability) noexcept {
  set_structure(x, y, 0, id, durability);
}

void Map::set_structure(int x, int y, int z, Structure::ID id, uint8_t durability) noexcept {
  set_structure_cell(x, y, z, Structure::Cell{id, durability});
}

void Map::set_structure_cell(int x, int y, int z, Structure::Cell cell) noexcept {
  if (!in_bounds(x, y)) return;
  if (z == 0) {
    surface_structure_grid(x, y) = cell;
  } else {
    vertical_structures.set(x, y, z, cell);
  }
}

Structure::Cell Map::get_structure(int x, int y, int z) const noexcept {
  if (!in_bounds(x, y)) {
    return {Structure::ID::None, 0};
  }
  if (z == 0) {
    return surface_structure_grid(x, y);
  }
  return vertical_structures.get(x, y, z);
}

bool Map::has_structure(int x, int y, int z) const noexcept {
  return get_structure(x, y, z).id != Structure::ID::None;
}

bool Map::remove_structure(int x, int y, int z) noexcept {
  if (!in_bounds(x, y)) return false;
  auto cur = get_structure(x, y, z);
  if (cur.id == Structure::ID::None) return false;

  bool was_load_bearing = Structure::getData(cur.id).isLoadBearing;
  set_structure_cell(x, y, z, {Structure::ID::None, 0});

  if (was_load_bearing) {
    trigger_cascade_check(x, y, z);
  }
  return true;
}

bool Map::damage_structure(int x, int y, int z, uint8_t damage) noexcept {
  if (!in_bounds(x, y)) return false;
  auto s = get_structure(x, y, z);
  if (s.id == Structure::ID::None) return false;

  if (damage >= s.durability) {
    auto data = Structure::getData(s.id);
    bool was_load_bearing = data.isLoadBearing;
    Structure::ID debris = data.debrisType;

    if (debris != Structure::ID::None) {
      set_structure_cell(x, y, z, {debris, Structure::getData(debris).maxDurability});
    } else {
      set_structure_cell(x, y, z, {Structure::ID::None, 0});
    }

    if (was_load_bearing) {
      trigger_cascade_check(x, y, z);
    }
    return true; // Destroyed
  } else {
    s.durability = static_cast<uint8_t>(s.durability - damage);
    set_structure_cell(x, y, z, s);
    return false; // Still standing
  }
}

bool Map::has_structural_support(int x, int y, int z) const noexcept {
  if (z <= 0) {
    return true; // Supported by natural terrain or bedrock
  }

  // Direct vertical support from structure below
  auto below = get_structure(x, y, z - 1);
  if (below.id != Structure::ID::None && Structure::getData(below.id).isLoadBearing) {
    return true;
  }

  // Check if any adjacent supporting column/wall exists within 1 tile underneath
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      if (dx == 0 && dy == 0) continue;
      int nx = x + dx;
      int ny = y + dy;
      if (in_bounds(nx, ny)) {
        auto adj_below = get_structure(nx, ny, z - 1);
        if (adj_below.id != Structure::ID::None && Structure::getData(adj_below.id).isLoadBearing) {
          return true;
        }
      }
    }
  }

  return false;
}

void Map::trigger_cascade_check(int x, int y, int z) noexcept {
  int upper_z = z + 1;
  for (int dy = -1; dy <= 1; ++dy) {
    for (int dx = -1; dx <= 1; ++dx) {
      int nx = x + dx;
      int ny = y + dy;
      if (!in_bounds(nx, ny)) continue;

      auto upper = get_structure(nx, ny, upper_z);
      if (upper.id != Structure::ID::None) {
        if (!has_structural_support(nx, ny, upper_z)) {
          // Unsupported structure collapses!
          auto data = Structure::getData(upper.id);
          bool was_load_bearing = data.isLoadBearing;
          Structure::ID debris = data.debrisType;

          // Upper cell becomes None (revealing OpenAir)
          set_structure_cell(nx, ny, upper_z, {Structure::ID::None, 0});

          // Lower cell receives falling debris
          Structure::ID floor_debris = (debris != Structure::ID::None) ? debris : Structure::ID::Rubble;
          set_structure_cell(nx, ny, z, {floor_debris, Structure::getData(floor_debris).maxDurability});

          // Cascade upwards if this was supporting higher stories
          if (was_load_bearing) {
            trigger_cascade_check(nx, ny, upper_z);
          }
        }
      }
    }
  }
}

void Map::set_vegetation(int x, int y, Vegetation::ID id, uint8_t amount) noexcept {
  if (in_bounds(x, y)) {
    vegetation_grid(x, y) = Vegetation::Cell{id, amount};
  }
}

Vegetation::Cell Map::get_vegetation(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return Vegetation::Cell{Vegetation::ID::None, 0};
  }
  return vegetation_grid(x, y);
}

bool Map::has_vegetation(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return false;
  }
  return vegetation_grid(x, y).id != Vegetation::ID::None;
}

HarvestResult Map::harvest_vegetation(int x, int y, int amount) noexcept {
  if (!in_bounds(x, y)) {
    return {};
  }

  auto &cell = vegetation_grid(x, y);
  if (cell.id == Vegetation::ID::None) {
    return {};
  }

  auto data = Vegetation::getData(cell.id);
  if (data.resourceType == Vegetation::ResourceType::None || cell.resource_amount == 0) {
    return {Vegetation::ResourceType::None, 0, false, data.name};
  }

  int gathered = std::min(static_cast<int>(cell.resource_amount), amount);
  cell.resource_amount = static_cast<uint8_t>(cell.resource_amount - gathered);
  bool depleted = (cell.resource_amount == 0);

  if (depleted) {
    // Transition to harvested state
    if (cell.id == Vegetation::ID::DeciduousTree || cell.id == Vegetation::ID::ConiferousTree) {
      cell.id = Vegetation::ID::TreeStump;
      cell.resource_amount = Vegetation::getData(Vegetation::ID::TreeStump).maxYield;
    } else if (cell.id == Vegetation::ID::BerryBush) {
      cell.id = Vegetation::ID::DepletedBush;
      cell.resource_amount = 0;
    } else {
      cell.id = Vegetation::ID::None;
      cell.resource_amount = 0;
    }
  }

  return {data.resourceType, gathered, depleted, data.name};
}

void Map::set_region(int x, int y, World::RegionID id) noexcept {
  if (in_bounds(x, y)) {
    region_grid(x, y) = id;
  }
}

World::RegionID Map::get_region(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return World::RegionID::LowlandMeadow;
  }
  return region_grid(x, y);
}

Climate::WeatherData Map::get_weather(int x, int y) const noexcept {
  World::RegionID reg = get_region(x, y);
  if (climate_gradient.is_initialized() && in_bounds(x, y)) {
    Climate::WeatherID wid = climate_gradient.sample_weather_id(x, y);
    auto data = Climate::getWeatherData(wid);
    data.temperature_celsius = climate_gradient.sample_temperature_celsius(x, y);
    data.humidity_pct = climate_gradient.sample_humidity_pct(x, y);
    return data;
  }
  auto region_data = World::getRegionData(reg);
  return Climate::getWeatherData(region_data.default_weather);
}

void Map::init_climate(std::vector<float> macro_elev, std::vector<float> macro_moist, std::vector<float> macro_temp) {
  climate_gradient.init(width, height, std::move(macro_elev), std::move(macro_moist), std::move(macro_temp));
}

float Map::get_temperature(int x, int y) const noexcept {
  if (climate_gradient.is_initialized() && in_bounds(x, y)) {
    return climate_gradient.sample_temperature_celsius(x, y);
  }
  return get_weather(x, y).temperature_celsius;
}

float Map::get_humidity(int x, int y) const noexcept {
  if (climate_gradient.is_initialized() && in_bounds(x, y)) {
    return climate_gradient.sample_humidity_pct(x, y);
  }
  return get_weather(x, y).humidity_pct;
}

bool Map::in_bounds(int x, int y) const noexcept {
  return tile_grid.in_bounds(x, y);
}

bool Map::is_walkable(int x, int y, int z) const noexcept {
  if (!in_bounds(x, y)) {
    return false;
  }

  if (z == 0) {
    if (Tile::getData(tile_grid(x, y)).blocksMovement) {
      return false;
    }
    if (Vegetation::getData(vegetation_grid(x, y).id).blocksMovement) {
      return false;
    }
    auto s = surface_structure_grid(x, y);
    if (s.id != Structure::ID::None && Structure::getData(s.id).blocksMovement) {
      return false;
    }
    return true;
  } else if (z > 0) {
    // Upper level requires constructed floor or stairs to walk on
    auto s = get_structure(x, y, z);
    if (s.id == Structure::ID::None) {
      return false; // OpenAir is not walkable
    }
    return !Structure::getData(s.id).blocksMovement;
  } else {
    // Subterranean level: solid rock blocks unless excavated
    auto s = get_structure(x, y, z);
    if (s.id == Structure::ID::None) {
      return false; // Solid subterranean rock
    }
    return !Structure::getData(s.id).blocksMovement;
  }
}

bool Map::blocks_sight(int x, int y, int z) const noexcept {
  if (!in_bounds(x, y)) {
    return true;
  }

  if (z == 0) {
    if (Tile::getData(tile_grid(x, y)).blocksSight) {
      return true;
    }
    if (Vegetation::getData(vegetation_grid(x, y).id).blocksSight) {
      return true;
    }
    auto s = surface_structure_grid(x, y);
    if (s.id != Structure::ID::None) {
      return Structure::getData(s.id).blocksSight;
    }
    return false;
  } else if (z > 0) {
    auto s = get_structure(x, y, z);
    if (s.id == Structure::ID::None) {
      return false; // OpenAir does not block sight
    }
    return Structure::getData(s.id).blocksSight;
  } else {
    auto s = get_structure(x, y, z);
    if (s.id == Structure::ID::None) {
      return true; // Solid rock blocks sight
    }
    return Structure::getData(s.id).blocksSight;
  }
}

float Map::get_movement_cost(int x, int y, int z) const noexcept {
  if (!in_bounds(x, y)) {
    return 999.0f;
  }

  if (z == 0) {
    float base_cost = Tile::getData(tile_grid(x, y)).movementCost;
    float veg_mult = Vegetation::getData(vegetation_grid(x, y).id).movementCostMult;
    auto s = surface_structure_grid(x, y);
    float struct_mult = (s.id != Structure::ID::None) ? Structure::getData(s.id).movementCostMult : 1.0f;
    auto weather = get_weather(x, y);
    return base_cost * veg_mult * struct_mult * weather.movement_cost_mult;
  } else {
    auto s = get_structure(x, y, z);
    if (s.id == Structure::ID::None) {
      return 999.0f;
    }
    float struct_mult = Structure::getData(s.id).movementCostMult;
    auto weather = get_weather(x, y);
    return 1.0f * struct_mult * weather.movement_cost_mult;
  }
}

int Map::get_visibility_limit(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return 0;
  }
  return get_weather(x, y).visibility_limit;
}

bool Map::raycast_los(int x0, int y0, int x1, int y1, int z) const noexcept {
  return Vision::has_line_of_sight(*this, x0, y0, x1, y1, -1, z);
}

bool Map::can_stamp_building(int x, int y, int w, int h) const noexcept {
  if (!in_bounds(x, y) || !in_bounds(x + w - 1, y + h - 1)) {
    return false;
  }
  for (int dy = 0; dy < h; ++dy) {
    for (int dx = 0; dx < w; ++dx) {
      Tile::ID t = at(x + dx, y + dy, 0);
      if (t == Tile::ID::DeepWater || t == Tile::ID::MountainPeak || t == Tile::ID::Cliff) {
        return false;
      }
    }
  }
  return true;
}

bool Map::stamp_building(int x, int y, const Architecture::BuildingTemplate &prefab, int rotation) noexcept {
  if (!prefab.is_valid()) {
    return false;
  }

  int rot = ((rotation % 360) + 360) % 360;
  int orig_w = prefab.width;
  int orig_h = prefab.height;

  int target_w = (rot == 90 || rot == 270) ? orig_h : orig_w;
  int target_h = (rot == 90 || rot == 270) ? orig_w : orig_h;

  if (!in_bounds(x, y) || !in_bounds(x + target_w - 1, y + target_h - 1)) {
    return false;
  }

  auto stamp_layer = [&](int level, Structure::ID wall_t, Structure::ID floor_t, const std::vector<std::string> &layout) {
    if (layout.empty()) return;
    int f_orig_h = static_cast<int>(layout.size());

    for (int tr = 0; tr < target_h; ++tr) {
      for (int tc = 0; tc < target_w; ++tc) {
        int sr = 0;
        int sc = 0;
        if (rot == 0) {
          sr = tr;
          sc = tc;
        } else if (rot == 90) {
          sr = orig_h - 1 - tc;
          sc = tr;
        } else if (rot == 180) {
          sr = orig_h - 1 - tr;
          sc = orig_w - 1 - tc;
        } else if (rot == 270) {
          sr = tc;
          sc = orig_w - 1 - tr;
        }

        char glyph = ' ';
        if (sr >= 0 && sr < f_orig_h && sc >= 0 && sc < static_cast<int>(layout[sr].size())) {
          glyph = layout[sr][sc];
        }

        int world_x = x + tc;
        int world_y = y + tr;

        // On surface level, clear vegetation under building footprint
        if (level == 0) {
          set_vegetation(world_x, world_y, Vegetation::ID::None, 0);
        }

        Structure::ID sid = Structure::ID::None;
        switch (glyph) {
        case '#': sid = wall_t; break;
        case '.': sid = floor_t; break;
        case '+': sid = Structure::ID::DoorClosed; break;
        case '/': sid = Structure::ID::DoorOpen; break;
        case '"': sid = Structure::ID::Window; break;
        case '>': sid = Structure::ID::StairsDown; break;
        case '<': sid = Structure::ID::StairsUp; break;
        case 'H': sid = Structure::ID::Ladder; break;
        case '&': sid = Structure::ID::Anvil; break;
        case '=': sid = Structure::ID::Counter; break;
        case ' ': sid = Structure::ID::None; break;
        default:  sid = floor_t; break;
        }

        if (sid != Structure::ID::None) {
          uint8_t max_hp = Structure::getData(sid).maxDurability;
          set_structure(world_x, world_y, level, sid, max_hp);
        }
      }
    }
  };

  if (!prefab.floors.empty()) {
    for (const auto &floor : prefab.floors) {
      stamp_layer(floor.level, floor.wall_type, floor.floor_type, floor.layout);
    }
  } else if (!prefab.layout.empty()) {
    stamp_layer(0, prefab.wall_type, prefab.floor_type, prefab.layout);
  }

  return true;
}

bool Map::open_door(int x, int y, int z) noexcept {
  if (!in_bounds(x, y)) return false;
  auto s = get_structure(x, y, z);
  if (s.id == Structure::ID::DoorClosed) {
    set_structure(x, y, z, Structure::ID::DoorOpen, s.durability);
    return true;
  }
  return false;
}

bool Map::close_door(int x, int y, int z) noexcept {
  if (!in_bounds(x, y)) return false;
  auto s = get_structure(x, y, z);
  if (s.id == Structure::ID::DoorOpen) {
    set_structure(x, y, z, Structure::ID::DoorClosed, s.durability);
    return true;
  }
  return false;
}

} // namespace GameMap
