#pragma once
#include "region.hpp"
#include "spatial_grid.hpp"
#include "structure.hpp"
#include "tile.hpp"
#include "vegetation.hpp"
#include "weather.hpp"
#include <unordered_map>

namespace Architecture {
struct BuildingTemplate;
}

namespace GameMap {

struct HarvestResult {
  Vegetation::ResourceType resource{Vegetation::ResourceType::None};
  int amount_gathered{0};
  bool depleted{false};
  std::string_view plant_name{"none"};
};

struct ChunkCoord {
  int cx{0};
  int cy{0};
  int z{0};
  bool operator==(const ChunkCoord &) const = default;
};

struct ChunkHash {
  size_t operator()(const ChunkCoord &c) const noexcept {
    return (static_cast<size_t>(c.cx) * 73856093u) ^
           (static_cast<size_t>(c.cy) * 19349663u) ^
           (static_cast<size_t>(c.z) * 83492791u);
  }
};

class SparseVerticalStorage {
public:
  static constexpr int CHUNK_SIZE = 32;

private:
  std::unordered_map<ChunkCoord, SpatialGrid2D<Structure::Cell>, ChunkHash> chunks;

public:
  SparseVerticalStorage() = default;

  [[nodiscard]] Structure::Cell get(int x, int y, int z) const noexcept {
    int cx = x >= 0 ? (x / CHUNK_SIZE) : ((x - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int cy = y >= 0 ? (y / CHUNK_SIZE) : ((y - CHUNK_SIZE + 1) / CHUNK_SIZE);
    auto it = chunks.find({cx, cy, z});
    if (it == chunks.end()) {
      return {Structure::ID::None, 0};
    }
    int lx = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = ((y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    return it->second(lx, ly);
  }

  void set(int x, int y, int z, Structure::Cell cell) {
    int cx = x >= 0 ? (x / CHUNK_SIZE) : ((x - CHUNK_SIZE + 1) / CHUNK_SIZE);
    int cy = y >= 0 ? (y / CHUNK_SIZE) : ((y - CHUNK_SIZE + 1) / CHUNK_SIZE);
    ChunkCoord coord{cx, cy, z};
    auto it = chunks.find(coord);
    if (it == chunks.end()) {
      if (cell.id == Structure::ID::None) return; // Don't allocate for empty cell
      auto [new_it, _] = chunks.emplace(coord, SpatialGrid2D<Structure::Cell>(CHUNK_SIZE, CHUNK_SIZE, Structure::Cell{Structure::ID::None, 0}));
      it = new_it;
    }
    int lx = ((x % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    int ly = ((y % CHUNK_SIZE) + CHUNK_SIZE) % CHUNK_SIZE;
    it->second(lx, ly) = cell;
  }

  [[nodiscard]] bool has(int x, int y, int z) const noexcept {
    return get(x, y, z).id != Structure::ID::None;
  }

  void clear() noexcept {
    chunks.clear();
  }
};

class Map {
private:
  int width{0};
  int height{0};
  SpatialGrid2D<Tile::ID> tile_grid;
  SpatialGrid2D<World::RegionID> region_grid;
  SpatialGrid2D<Vegetation::Cell> vegetation_grid;
  SpatialGrid2D<Structure::Cell> surface_structure_grid;
  SparseVerticalStorage vertical_structures;
  Climate::GradientMap climate_gradient;

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
  [[nodiscard]] Tile::ID at(int x, int y, int z = 0) const noexcept;

  // Structure accessors & mutators across Z-levels
  void set_structure(int x, int y, Structure::ID id, uint8_t durability = 100) noexcept;
  void set_structure(int x, int y, int z, Structure::ID id, uint8_t durability = 100) noexcept;
  void set_structure_cell(int x, int y, int z, Structure::Cell cell) noexcept;
  [[nodiscard]] Structure::Cell get_structure(int x, int y, int z = 0) const noexcept;
  [[nodiscard]] bool has_structure(int x, int y, int z = 0) const noexcept;
  bool remove_structure(int x, int y, int z = 0) noexcept;
  bool damage_structure(int x, int y, int z, uint8_t damage) noexcept;

  // Support & cascade destruction
  [[nodiscard]] bool has_structural_support(int x, int y, int z) const noexcept;
  void trigger_cascade_check(int x, int y, int z) noexcept;

  // Vegetation accessors & resource harvesting
  void set_vegetation(int x, int y, Vegetation::ID id, uint8_t amount = 100) noexcept;
  [[nodiscard]] Vegetation::Cell get_vegetation(int x, int y) const noexcept;
  [[nodiscard]] bool has_vegetation(int x, int y) const noexcept;
  HarvestResult harvest_vegetation(int x, int y, int amount = 25) noexcept;

  // Region and climate accessors
  void set_region(int x, int y, World::RegionID id) noexcept;
  [[nodiscard]] World::RegionID get_region(int x, int y) const noexcept;
  [[nodiscard]] Climate::WeatherData get_weather(int x, int y) const noexcept;

  // Pre-generated continental climate gradient
  void init_climate(std::vector<float> macro_elev, std::vector<float> macro_moist, std::vector<float> macro_temp = {});
  [[nodiscard]] bool has_climate_gradient() const noexcept { return climate_gradient.is_initialized(); }
  [[nodiscard]] float get_temperature(int x, int y) const noexcept;
  [[nodiscard]] float get_humidity(int x, int y) const noexcept;
  [[nodiscard]] const Climate::GradientMap& get_climate_gradient() const noexcept { return climate_gradient; }

  // Architectural stamping & door interaction
  [[nodiscard]] bool can_stamp_building(int x, int y, int w, int h) const noexcept;
  bool stamp_building(int x, int y, const Architecture::BuildingTemplate &prefab, int rotation = 0) noexcept;
  bool open_door(int x, int y, int z = 0) noexcept;
  bool close_door(int x, int y, int z = 0) noexcept;

  // Spatial queries
  [[nodiscard]] bool in_bounds(int x, int y) const noexcept;
  [[nodiscard]] bool is_walkable(int x, int y, int z = 0) const noexcept;
  [[nodiscard]] bool blocks_sight(int x, int y, int z = 0) const noexcept;
  [[nodiscard]] float get_movement_cost(int x, int y, int z = 0) const noexcept;
  [[nodiscard]] int get_visibility_limit(int x, int y) const noexcept;

  // Line-of-sight raycasting (Bresenham algorithm)
  [[nodiscard]] bool raycast_los(int x0, int y0, int x1, int y1, int z = 0) const noexcept;

  // Dimensions
  [[nodiscard]] int get_width() const noexcept { return width; }
  [[nodiscard]] int get_height() const noexcept { return height; }

  // High-performance direct row / scanline pointers for batch operations
  Tile::ID* tile_row(int y) noexcept { return tile_grid.row_data(static_cast<size_t>(y)); }
  const Tile::ID* tile_row(int y) const noexcept { return tile_grid.row_data(static_cast<size_t>(y)); }

  World::RegionID* region_row(int y) noexcept { return region_grid.row_data(static_cast<size_t>(y)); }
  const World::RegionID* region_row(int y) const noexcept { return region_grid.row_data(static_cast<size_t>(y)); }

  Vegetation::Cell* vegetation_row(int y) noexcept { return vegetation_grid.row_data(static_cast<size_t>(y)); }
  const Vegetation::Cell* vegetation_row(int y) const noexcept { return vegetation_grid.row_data(static_cast<size_t>(y)); }

  Structure::Cell* structure_row(int y) noexcept { return surface_structure_grid.row_data(static_cast<size_t>(y)); }
  const Structure::Cell* structure_row(int y) const noexcept { return surface_structure_grid.row_data(static_cast<size_t>(y)); }

  [[nodiscard]] const SpatialGrid2D<Tile::ID>& get_tile_grid() const noexcept { return tile_grid; }
  [[nodiscard]] const SpatialGrid2D<World::RegionID>& get_region_grid() const noexcept { return region_grid; }
  [[nodiscard]] const SpatialGrid2D<Vegetation::Cell>& get_vegetation_grid() const noexcept { return vegetation_grid; }
  [[nodiscard]] const SpatialGrid2D<Structure::Cell>& get_structure_grid() const noexcept { return surface_structure_grid; }
};

// Aliases
using map = Map;
using WorldMap = Map;

} // namespace GameMap
