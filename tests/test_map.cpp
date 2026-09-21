#include "test_common.hpp"
#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../region.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"
#include "../weather.hpp"

#include <chrono>
#include <iomanip>
#include <iostream>

void test_spatial_grid_10k() {
    GameMap::Map world(10000, 10000);

    CHECK(world.get_width() == 10000);
    CHECK(world.get_height() == 10000);

    // Bounds checking
    CHECK(world.in_bounds(0, 0));
    CHECK(world.in_bounds(9999, 9999));
    CHECK(!world.in_bounds(-1, 500));
    CHECK(!world.in_bounds(500, -1));
    CHECK(!world.in_bounds(10000, 500));
    CHECK(!world.in_bounds(500, 10000));

    // Test static weather retrieval through map
    world.set_region(2500, 1500, World::RegionID::AridWaste);
    auto w1 = world.get_weather(2500, 1500);
    CHECK(w1.name == "Scorching Sun");

    world.set_region(7500, 8500, World::RegionID::GlacialCrown);
    auto w2 = world.get_weather(7500, 8500);
    CHECK(w2.name == "Whiteout Blizzard");

    // Test movement cost combining tile, vegetation, and weather
    world.set(100, 100, Tile::ID::ForestSoil);
    world.set_region(100, 100, World::RegionID::LowlandMeadow);
    world.set_vegetation(100, 100, Vegetation::ID::None);
    float cost_bare = world.get_movement_cost(100, 100);
    CHECK(cost_bare > 1.0f);

    // Add DenseScrub vegetation (cost mult 1.6)
    world.set_vegetation(100, 100, Vegetation::ID::DenseScrub, 60);
    float cost_scrub = world.get_movement_cost(100, 100);
    CHECK(cost_scrub > cost_bare * 1.5f);

    // Test Line of Sight (Raycasting) through clear ground
    world.set(500, 500, Tile::ID::Grassland);
    world.set(500, 501, Tile::ID::Grassland);
    world.set(500, 502, Tile::ID::Grassland);
    world.set(500, 503, Tile::ID::Grassland);
    world.set_vegetation(500, 500, Vegetation::ID::None);
    world.set_vegetation(500, 501, Vegetation::ID::None);
    world.set_vegetation(500, 502, Vegetation::ID::None);
    world.set_vegetation(500, 503, Vegetation::ID::None);
    world.set_region(500, 500, World::RegionID::LowlandMeadow);
    CHECK(world.raycast_los(500, 500, 500, 503) == true);

    // Obstruct with a standing DeciduousTree on cell (500, 502)
    world.set_vegetation(500, 502, Vegetation::ID::DeciduousTree, 100);
    CHECK(world.raycast_los(500, 500, 500, 503) == false);

    // Harvest the tree down to a stump: sight is unblocked!
    world.harvest_vegetation(500, 502, 100);
    CHECK(world.get_vegetation(500, 502).id == Vegetation::ID::TreeStump);
    CHECK(world.raycast_los(500, 500, 500, 503) == true);

    // Weather visibility limit clipping
    world.set_region(500, 500, World::RegionID::GlacialCrown);
    CHECK(world.raycast_los(500, 500, 500, 510) == false);
}

void test_continental_generation() {
    GameMap::Map world(10000, 10000);

    auto t0 = std::chrono::high_resolution_clock::now();
    auto spawn = GameMap::MapGenerator::generate(world, 1337);
    auto t1 = std::chrono::high_resolution_clock::now();

    double gen_sec = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "Map generation time: " << std::fixed << std::setprecision(3) << gen_sec << "s (100M cells)\n";

    // Spawn point validation
    CHECK(world.in_bounds(spawn.x, spawn.y));
    CHECK(world.is_walkable(spawn.x, spawn.y));

    // Road center should have no vegetation
    auto road_y = world.get_height() / 2;
    CHECK(world.get_vegetation(world.get_width() / 2, road_y).id == Vegetation::ID::None);
}

int main() {
    test_spatial_grid_10k();
    test_continental_generation();
    std::cout << "All map tests passed.\n";
    return 0;
}
