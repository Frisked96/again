#include "map_generator.hpp"
#include <algorithm>
#include <cmath>
#include <vector>

namespace GameMap {

namespace {

// Fast deterministic integer hash for local micro-variance
inline uint32_t hash2d(uint32_t x, uint32_t y, uint32_t seed) noexcept {
  uint32_t h = (x * 374761393u) + (y * 668265263u) + (seed * 982451653u);
  h = (h ^ (h >> 13)) * 1274126177u;
  return h ^ (h >> 16);
}

inline float hash_float(uint32_t x, uint32_t y, uint32_t seed) noexcept {
  return static_cast<float>(hash2d(x, y, seed) & 0xFFFFu) / 65535.0f;
}

// Cubic smoothstep for macro interpolation
inline float smoothstep(float t) noexcept {
  return t * t * (3.0f - 2.0f * t);
}

} // namespace

SpawnPoint MapGenerator::generate(Map &map, uint32_t seed) {
  const int width = map.get_width();
  const int height = map.get_height();

  // 1. Setup macro-grid (32 x 32 control points across the 10,000 x 10,000 expanse)
  constexpr int MACRO_DIM = 32;
  std::vector<float> macro_elev(MACRO_DIM * MACRO_DIM);
  std::vector<float> macro_moist(MACRO_DIM * MACRO_DIM);

  for (int my = 0; my < MACRO_DIM; ++my) {
    for (int mx = 0; mx < MACRO_DIM; ++mx) {
      int idx = my * MACRO_DIM + mx;

      // Base noise
      float e = hash_float(static_cast<uint32_t>(mx), static_cast<uint32_t>(my), seed + 101);
      float m = hash_float(static_cast<uint32_t>(mx), static_cast<uint32_t>(my), seed + 202);

      // Shape continental landmass:
      // Mountain spine running through upper-middle diagonal
      float diag = std::abs((static_cast<float>(mx) - static_cast<float>(my) * 0.7f) / static_cast<float>(MACRO_DIM) - 0.2f);
      if (diag < 0.15f) {
        e = std::min(1.0f, e * 0.4f + 0.65f + (0.15f - diag) * 2.0f);
      }

      // Coastal margins along western & southern borders
      float dist_to_west = static_cast<float>(mx) / static_cast<float>(MACRO_DIM);
      if (dist_to_west < 0.12f) {
        e *= (dist_to_west / 0.12f);
        m = std::min(1.0f, m + 0.4f);
      }

      macro_elev[idx] = std::clamp(e, 0.0f, 1.0f);
      macro_moist[idx] = std::clamp(m, 0.0f, 1.0f);
    }
  }

  // 2. Generate 100M cells row-by-row sequentially for maximum memory bandwidth
  const float step_x = static_cast<float>(MACRO_DIM - 1) / static_cast<float>(std::max(1, width - 1));
  const float step_y = static_cast<float>(MACRO_DIM - 1) / static_cast<float>(std::max(1, height - 1));

  for (int y = 0; y < height; ++y) {
    Tile::ID* tile_row = map.tile_row(y);
    World::RegionID* region_row = map.region_row(y);

    float gy = static_cast<float>(y) * step_y;
    int my0 = static_cast<int>(gy);
    int my1 = std::min(MACRO_DIM - 1, my0 + 1);
    float ty = smoothstep(gy - static_cast<float>(my0));

    // Latitude temperature gradient: North (cold, y=0) to South (hot, y=height)
    float latitude_temp = 0.05f + 0.85f * (static_cast<float>(y) / static_cast<float>(height));

    for (int x = 0; x < width; ++x) {
      float gx = static_cast<float>(x) * step_x;
      int mx0 = static_cast<int>(gx);
      int mx1 = std::min(MACRO_DIM - 1, mx0 + 1);
      float tx = smoothstep(gx - static_cast<float>(mx0));

      // Bilinear interpolation of macro elevation and moisture
      float e00 = macro_elev[my0 * MACRO_DIM + mx0];
      float e10 = macro_elev[my0 * MACRO_DIM + mx1];
      float e01 = macro_elev[my1 * MACRO_DIM + mx0];
      float e11 = macro_elev[my1 * MACRO_DIM + mx1];
      float elev = (1.0f - ty) * ((1.0f - tx) * e00 + tx * e10) +
                   ty * ((1.0f - tx) * e01 + tx * e11);

      float m00 = macro_moist[my0 * MACRO_DIM + mx0];
      float m10 = macro_moist[my0 * MACRO_DIM + mx1];
      float m01 = macro_moist[my1 * MACRO_DIM + mx0];
      float m11 = macro_moist[my1 * MACRO_DIM + mx1];
      float moist = (1.0f - ty) * ((1.0f - tx) * m00 + tx * m10) +
                    ty * ((1.0f - tx) * m01 + tx * m11);

      // Elevation lapse rate: higher elevations are significantly colder
      float effective_temp = std::clamp(latitude_temp - 0.45f * elev, 0.0f, 1.0f);

      // Local micro-noise for natural terrain variation
      uint32_t local_h = hash2d(static_cast<uint32_t>(x), static_cast<uint32_t>(y), seed);
      float local_dither = static_cast<float>(local_h & 0xFFu) / 255.0f;

      World::RegionID region;
      Tile::ID tile;

      // 3. Whittaker Biome Classification & Terrain Assignment
      if (elev < 0.08f) {
        // Ocean / Coastline
        region = World::RegionID::CoastalCliffs;
        tile = (elev < 0.05f) ? Tile::ID::DeepWater : Tile::ID::CoastalWater;
      } else if (elev >= 0.82f) {
        // High Alpine Crags & Glaciers
        if (effective_temp < 0.20f) {
          region = World::RegionID::GlacialCrown;
          tile = (local_dither < 0.4f) ? Tile::ID::Ice :
                 (local_dither < 0.7f) ? Tile::ID::PackIce : Tile::ID::SnowDeep;
        } else {
          region = World::RegionID::HighlandPeaks;
          tile = (local_dither < 0.5f) ? Tile::ID::MountainPeak : Tile::ID::Cliff;
        }
      } else if (elev >= 0.68f) {
        // Foothills & High Scree
        region = World::RegionID::HighlandPeaks;
        tile = (local_dither < 0.55f) ? Tile::ID::Foothills : Tile::ID::RockyGround;
      } else if (effective_temp < 0.25f) {
        // Cold Boreal & Tundra
        if (moist < 0.40f) {
          region = World::RegionID::FrostTundra;
          tile = (local_dither < 0.6f) ? Tile::ID::Tundra : Tile::ID::SnowLight;
        } else {
          region = World::RegionID::BorealTaiga;
          tile = (local_dither < 0.65f) ? Tile::ID::ForestConiferous : Tile::ID::SnowLight;
        }
      } else if (effective_temp > 0.68f) {
        // Arid Wastes & Steppes
        if (moist < 0.35f) {
          region = World::RegionID::AridWaste;
          tile = (local_dither < 0.65f) ? Tile::ID::DesertSand : Tile::ID::HardenedClay;
        } else if (moist < 0.60f) {
          region = World::RegionID::DrySteppe;
          tile = (local_dither < 0.6f) ? Tile::ID::DrySteppe : Tile::ID::HardenedClay;
        } else {
          region = World::RegionID::LowlandMeadow;
          tile = (local_dither < 0.75f) ? Tile::ID::Grassland : Tile::ID::Farmland;
        }
      } else {
        // Temperate Continental Zone
        if (moist > 0.72f && elev < 0.32f) {
          region = World::RegionID::PeatFen;
          tile = (local_dither < 0.50f) ? Tile::ID::PeatBog :
                 (local_dither < 0.80f) ? Tile::ID::DenseScrub : Tile::ID::MarshWater;
        } else if (moist > 0.45f) {
          region = World::RegionID::DeciduousWeald;
          tile = (local_dither < 0.60f) ? Tile::ID::ForestDeciduous :
                 (local_dither < 0.85f) ? Tile::ID::DenseScrub : Tile::ID::Grassland;
        } else {
          region = World::RegionID::LowlandMeadow;
          tile = (local_dither < 0.60f) ? Tile::ID::Grassland :
                 (local_dither < 0.85f) ? Tile::ID::Farmland : Tile::ID::RockyGround;
        }
      }

      region_row[x] = region;
      tile_row[x] = tile;
    }
  }

  // 4. Lay medieval road corridor across the central temperate zone
  const int road_y = height / 2;
  for (int x = 0; x < width; ++x) {
    // Slight meander
    int ry = road_y + static_cast<int>(std::sin(static_cast<float>(x) * 0.015f) * 12.0f);
    if (map.in_bounds(x, ry)) {
      Tile::ID cur = map.at(x, ry);
      if (cur != Tile::ID::DeepWater && cur != Tile::ID::MountainPeak && cur != Tile::ID::Cliff) {
        if (cur == Tile::ID::River || cur == Tile::ID::MarshWater || cur == Tile::ID::CoastalWater) {
          map.set(x, ry, Tile::ID::RiverFord);
        } else {
          map.set(x, ry, (x % 3 == 0) ? Tile::ID::Cobblestone : Tile::ID::DirtRoad);
        }
      }
    }
  }

  // 5. Establish a guaranteed safe spawn point (preferring Lowland Meadow on a road or path)
  for (int radius = 0; radius < 1000; radius += 5) {
    for (int dy = -radius; dy <= radius; dy += 10) {
      for (int dx = -radius; dx <= radius; dx += 10) {
        int sx = (width / 2) + dx;
        int sy = road_y + dy;
        if (map.in_bounds(sx, sy) && map.is_walkable(sx, sy)) {
          World::RegionID reg = map.get_region(sx, sy);
          if (reg == World::RegionID::LowlandMeadow || reg == World::RegionID::DeciduousWeald) {
            return {sx, sy};
          }
        }
      }
    }
  }

  // Fallback to any walkable tile near center
  for (int dy = -50; dy <= 50; ++dy) {
    for (int dx = -50; dx <= 50; ++dx) {
      int sx = (width / 2) + dx;
      int sy = road_y + dy;
      if (map.in_bounds(sx, sy) && map.is_walkable(sx, sy)) {
        return {sx, sy};
      }
    }
  }

  return {width / 2, road_y};
}

} // namespace GameMap
