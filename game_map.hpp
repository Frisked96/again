#pragma once
#include "region.hpp"
#include "spatial_grid.hpp"
#include "tile.hpp"
#include "vegetation.hpp"
#include "weather.hpp"

namespace GameMap {

struct HarvestResult {
  Vegetation::ResourceType resource{Vegetation::ResourceType::None};
  int amount_gathered{0};
  bool depleted{false};
  std::string_view plant_name{"none"};
};

class Map {
private:
  int width{0};
  int height{0};
  SpatialGrid2D<Tile::ID> tile_grid;
  SpatialGrid2D<World::RegionID> region_grid;
  SpatialGrid2D<Vegetation::Cell> vegetation_grid;

public:
  Map(int w = 10000, int h = 10000,
      Tile::ID default_tile = Tile::ID::Grassland,
      World::RegionID default_region = World::RegionID::LowlandMeadow);

  void resize(int w, int h,
              Tile::ID default_tile = Tile::ID::Grassland,
              World::RegionID default_region = World::RegionID::LowlandMeadow);
  void clear(Tile::ID fill_tile = Tile::ID::Grassland,
             World::RegionID fill_region = World::RegionID::LowlandMeadow,
             Vegetation::ID fill_veg = Vegetation::ID::None);

  // Core tile accessors
  void set(int x, int y, Tile::ID id) noexcept;
  Tile::ID at(int x, int y) const noexcept;

  // Vegetation accessors & resource harvesting
  void set_vegetation(int x, int y, Vegetation::ID id, uint8_t amount = 100) noexcept;
  Vegetation::Cell get_vegetation(int x, int y) const noexcept;
  bool has_vegetation(int x, int y) const noexcept;
  HarvestResult harvest_vegetation(int x, int y, int amount = 25) noexcept;

  // Region and static climate accessors
  void set_region(int x, int y, World::RegionID id) noexcept;
  World::RegionID get_region(int x, int y) const noexcept;
  Climate::WeatherData get_weather(int x, int y) const noexcept;

  // Spatial queries
  bool in_bounds(int x, int y) const noexcept;
  bool is_walkable(int x, int y) const noexcept;
  bool blocks_sight(int x, int y) const noexcept;
  float get_movement_cost(int x, int y) const noexcept;
  int get_visibility_limit(int x, int y) const noexcept;

  // Line-of-sight raycasting (Bresenham algorithm)
  bool raycast_los(int x0, int y0, int x1, int y1) const noexcept;

  // Dimensions
  int get_width() const noexcept { return width; }
  int get_height() const noexcept { return height; }

  // High-performance direct row / scanline pointers for batch operations
  Tile::ID* tile_row(int y) noexcept { return tile_grid.row_data(static_cast<size_t>(y)); }
  const Tile::ID* tile_row(int y) const noexcept { return tile_grid.row_data(static_cast<size_t>(y)); }

  World::RegionID* region_row(int y) noexcept { return region_grid.row_data(static_cast<size_t>(y)); }
  const World::RegionID* region_row(int y) const noexcept { return region_grid.row_data(static_cast<size_t>(y)); }

  Vegetation::Cell* vegetation_row(int y) noexcept { return vegetation_grid.row_data(static_cast<size_t>(y)); }
  const Vegetation::Cell* vegetation_row(int y) const noexcept { return vegetation_grid.row_data(static_cast<size_t>(y)); }

  const SpatialGrid2D<Tile::ID>& get_tile_grid() const noexcept { return tile_grid; }
  const SpatialGrid2D<World::RegionID>& get_region_grid() const noexcept { return region_grid; }
  const SpatialGrid2D<Vegetation::Cell>& get_vegetation_grid() const noexcept { return vegetation_grid; }
};

// Aliases
using map = Map;
using WorldMap = Map;

} // namespace GameMap
