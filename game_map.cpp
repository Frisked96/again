#include "game_map.hpp"
#include "building_prefab.hpp"
#include "vision.hpp"
#include <cmath>
#include <cstdlib>

namespace GameMap {

Map::Map(int w, int h, Tile::ID default_tile, World::RegionID default_region)
    : width(w), height(h),
      tile_grid(static_cast<size_t>(w), static_cast<size_t>(h), default_tile),
      region_grid(static_cast<size_t>(w), static_cast<size_t>(h), default_region),
      vegetation_grid(static_cast<size_t>(w), static_cast<size_t>(h), Vegetation::Cell{Vegetation::ID::None, 0}) {}

void Map::resize(int w, int h, Tile::ID default_tile, World::RegionID default_region) {
  width = w;
  height = h;
  tile_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), default_tile);
  region_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), default_region);
  vegetation_grid.resize(static_cast<size_t>(w), static_cast<size_t>(h), Vegetation::Cell{Vegetation::ID::None, 0});
}

void Map::clear(Tile::ID fill_tile, World::RegionID fill_region, Vegetation::ID fill_veg) {
  tile_grid.fill(fill_tile);
  region_grid.fill(fill_region);
  uint8_t yield = (fill_veg == Vegetation::ID::None) ? 0 : Vegetation::getData(fill_veg).maxYield;
  vegetation_grid.fill(Vegetation::Cell{fill_veg, yield});
}

void Map::set(int x, int y, Tile::ID id) noexcept {
  if (in_bounds(x, y)) {
    tile_grid(x, y) = id;
  }
}

Tile::ID Map::at(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return Tile::ID::Void;
  }
  return tile_grid(x, y);
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

bool Map::is_walkable(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return false;
  }
  if (Tile::getData(tile_grid(x, y)).blocksMovement) {
    return false;
  }
  return !Vegetation::getData(vegetation_grid(x, y).id).blocksMovement;
}

bool Map::blocks_sight(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return true;
  }
  if (Tile::getData(tile_grid(x, y)).blocksSight) {
    return true;
  }
  return Vegetation::getData(vegetation_grid(x, y).id).blocksSight;
}

float Map::get_movement_cost(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return 999.0f;
  }
  float base_cost = Tile::getData(tile_grid(x, y)).movementCost;
  float veg_mult = Vegetation::getData(vegetation_grid(x, y).id).movementCostMult;
  auto weather = get_weather(x, y);
  return base_cost * veg_mult * weather.movement_cost_mult;
}

int Map::get_visibility_limit(int x, int y) const noexcept {
  if (!in_bounds(x, y)) {
    return 0;
  }
  return get_weather(x, y).visibility_limit;
}

bool Map::raycast_los(int x0, int y0, int x1, int y1) const noexcept {
  return Vision::has_line_of_sight(*this, x0, y0, x1, y1);
}

bool Map::can_stamp_building(int x, int y, int w, int h) const noexcept {
  if (!in_bounds(x, y) || !in_bounds(x + w - 1, y + h - 1)) {
    return false;
  }
  for (int dy = 0; dy < h; ++dy) {
    for (int dx = 0; dx < w; ++dx) {
      Tile::ID t = at(x + dx, y + dy);
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
      if (sr >= 0 && sr < orig_h && sc >= 0 && sc < static_cast<int>(prefab.layout[sr].size())) {
        glyph = prefab.layout[sr][sc];
      }

      int world_x = x + tc;
      int world_y = y + tr;

      // Always clear vegetation under building footprint
      set_vegetation(world_x, world_y, Vegetation::ID::None, 0);

      // Translate prefab glyph to Tile::ID
      switch (glyph) {
      case '#':
        set(world_x, world_y, prefab.wall_type);
        break;
      case '.':
        set(world_x, world_y, prefab.floor_type);
        break;
      case '+':
        set(world_x, world_y, Tile::ID::DoorClosed);
        break;
      case '/':
        set(world_x, world_y, Tile::ID::DoorOpen);
        break;
      case '"':
        set(world_x, world_y, Tile::ID::Window);
        break;
      case '>':
        set(world_x, world_y, Tile::ID::StairsDown);
        break;
      case '&':
        set(world_x, world_y, Tile::ID::Anvil);
        break;
      case '=':
        set(world_x, world_y, Tile::ID::Counter);
        break;
      default:
        if (glyph != ' ') {
          set(world_x, world_y, prefab.floor_type);
        }
        break;
      }
    }
  }

  return true;
}

bool Map::open_door(int x, int y) noexcept {
  if (in_bounds(x, y) && at(x, y) == Tile::ID::DoorClosed) {
    set(x, y, Tile::ID::DoorOpen);
    return true;
  }
  return false;
}

bool Map::close_door(int x, int y) noexcept {
  if (in_bounds(x, y) && at(x, y) == Tile::ID::DoorOpen) {
    set(x, y, Tile::ID::DoorClosed);
    return true;
  }
  return false;
}

} // namespace GameMap
