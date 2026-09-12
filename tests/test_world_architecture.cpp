#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../weather.hpp"
#include "../region.hpp"
#include "../tile.hpp"

#include <cassert>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <sys/resource.h>

void print_memory_usage() {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        std::cout << "  Peak Resident Memory (RSS): "
                  << usage.ru_maxrss / 1024.0 << " MB\n";
    }
}

void test_tile_properties() {
    std::cout << "[TEST] Validating realistic medieval tiles...\n";

    // Test water bodies
    auto deep_water = Tile::getData(Tile::ID::DeepWater);
    assert(deep_water.blocksMovement == true);
    assert(deep_water.movementCost > 100.0f);

    auto coastal_water = Tile::getData(Tile::ID::CoastalWater);
    assert(coastal_water.blocksMovement == false);
    assert(coastal_water.movementCost > 1.0f);

    // Test vegetation
    auto forest = Tile::getData(Tile::ID::ForestDeciduous);
    assert(forest.blocksMovement == false);
    assert(forest.blocksSight == true);
    assert(forest.flammability > 0);

    auto grassland = Tile::getData(Tile::ID::Grassland);
    assert(grassland.blocksMovement == false);
    assert(grassland.blocksSight == false);
    assert(grassland.movementCost == 1.0f);

    // Test arid / desert
    auto sand = Tile::getData(Tile::ID::DesertSand);
    assert(sand.blocksMovement == false);
    assert(sand.movementCost > 1.0f);

    // Test snow & ice
    auto snow_deep = Tile::getData(Tile::ID::SnowDeep);
    assert(snow_deep.blocksMovement == false);
    assert(snow_deep.movementCost > 2.0f);

    // Test alpine crags
    auto summit = Tile::getData(Tile::ID::MountainPeak);
    assert(summit.blocksMovement == true);
    assert(summit.blocksSight == true);

    std::cout << "  ✓ All tile physical properties verified successfully.\n";
}

void test_weather_and_regions() {
    std::cout << "[TEST] Validating realistic weather & static regional microclimates...\n";

    // Test weather data lookup
    auto scorching = Climate::getWeatherData(Climate::WeatherID::ScorchingArid);
    assert(scorching.temperature_celsius > 30.0f);
    assert(scorching.humidity_pct < 20.0f);

    auto blizzard = Climate::getWeatherData(Climate::WeatherID::Blizzard);
    assert(blizzard.temperature_celsius < -10.0f);
    assert(blizzard.visibility_limit <= 5);
    assert(blizzard.movement_cost_mult > 1.5f);

    auto mist = Climate::getWeatherData(Climate::WeatherID::DenseMist);
    assert(mist.humidity_pct == 100.0f);
    assert(mist.visibility_limit <= 6);

    // Test region to weather bindings
    auto waste_region = World::getRegionData(World::RegionID::AridWaste);
    assert(waste_region.default_weather == Climate::WeatherID::ScorchingArid);

    auto glacial_region = World::getRegionData(World::RegionID::GlacialCrown);
    assert(glacial_region.default_weather == Climate::WeatherID::Blizzard);

    auto fen_region = World::getRegionData(World::RegionID::PeatFen);
    assert(fen_region.default_weather == Climate::WeatherID::DenseMist);

    std::cout << "  ✓ Region-to-weather static bindings verified successfully.\n";
}

void test_spatial_grid_10k() {
    std::cout << "[TEST] Allocating 10,000 x 10,000 WorldMap (100,000,000 cells)...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    GameMap::Map world(10000, 10000);

    auto t1 = std::chrono::high_resolution_clock::now();
    double alloc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "  ✓ 100M cell map allocated in " << std::fixed << std::setprecision(2) << alloc_ms << " ms (~200 MB RAM).\n";

    assert(world.get_width() == 10000);
    assert(world.get_height() == 10000);

    // Bounds checking
    assert(world.in_bounds(0, 0));
    assert(world.in_bounds(9999, 9999));
    assert(!world.in_bounds(-1, 500));
    assert(!world.in_bounds(500, -1));
    assert(!world.in_bounds(10000, 500));
    assert(!world.in_bounds(500, 10000));

    // Test static weather retrieval through map
    world.set_region(2500, 1500, World::RegionID::AridWaste);
    auto w1 = world.get_weather(2500, 1500);
    assert(w1.name == "Scorching Sun");

    world.set_region(7500, 8500, World::RegionID::GlacialCrown);
    auto w2 = world.get_weather(7500, 8500);
    assert(w2.name == "Whiteout Blizzard");

    // Test movement cost combining tile and weather
    world.set(100, 100, Tile::ID::Grassland);
    world.set_region(100, 100, World::RegionID::LowlandMeadow);
    float cost_clear = world.get_movement_cost(100, 100);
    assert(cost_clear == 1.0f);

    world.set_region(100, 100, World::RegionID::GlacialCrown); // Blizzard mult = 1.6
    float cost_blizzard = world.get_movement_cost(100, 100);
    assert(std::abs(cost_blizzard - 1.6f) < 0.001f);

    // Test Line of Sight (Raycasting)
    world.set(500, 500, Tile::ID::Grassland);
    world.set(500, 501, Tile::ID::Grassland);
    world.set(500, 502, Tile::ID::Grassland);
    world.set(500, 503, Tile::ID::Grassland);
    world.set_region(500, 500, World::RegionID::LowlandMeadow); // Clear weather (visibility 60)
    assert(world.raycast_los(500, 500, 500, 503) == true);

    // Obstruct with MountainPeak
    world.set(500, 502, Tile::ID::MountainPeak);
    assert(world.raycast_los(500, 500, 500, 503) == false);

    // Weather visibility limit clipping
    world.set_region(500, 500, World::RegionID::GlacialCrown); // Blizzard max visibility = 4
    assert(world.raycast_los(500, 500, 500, 510) == false); // 10 tiles away exceeds visibility 4

    std::cout << "  ✓ Spatial methods (bounds, movement cost, raycast LoS) verified successfully.\n";
}

void test_continental_generation() {
    std::cout << "[TEST] Running continental medieval generator on 10,000 x 10,000 map...\n";
    GameMap::Map world(10000, 10000);

    auto t0 = std::chrono::high_resolution_clock::now();
    auto spawn = GameMap::MapGenerator::generate(world, 1337);
    auto t1 = std::chrono::high_resolution_clock::now();

    double gen_sec = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "  ✓ Generated 100,000,000 cells in " << std::fixed << std::setprecision(3) << gen_sec << " seconds!\n";

    // Spawn point validation
    std::cout << "  Spawn point: (" << spawn.x << ", " << spawn.y << ")\n";
    assert(world.in_bounds(spawn.x, spawn.y));
    assert(world.is_walkable(spawn.x, spawn.y));

    auto spawn_tile = Tile::getData(world.at(spawn.x, spawn.y));
    auto spawn_region = World::getRegionData(world.get_region(spawn.x, spawn.y));
    auto spawn_weather = world.get_weather(spawn.x, spawn.y);

    std::cout << "  Spawn standing on: " << spawn_tile.name << "\n";
    std::cout << "  Spawn region: " << spawn_region.name << "\n";
    std::cout << "  Spawn static climate: " << spawn_weather.name << " ("
              << spawn_weather.temperature_celsius << "°C, "
              << spawn_weather.humidity_pct << "% humidity)\n";

    // Sample different geographic zones to confirm diversity of static climates
    auto north_weather = world.get_weather(5000, 200);   // Arctic North
    auto south_weather = world.get_weather(5000, 9800);  // Arid South
    auto coast_weather = world.get_weather(200, 5000);   // Storm Coast West

    std::cout << "  Sampled North Climate (y=200): " << north_weather.name << "\n";
    std::cout << "  Sampled South Climate (y=9800): " << south_weather.name << "\n";
    std::cout << "  Sampled West Coast Climate (x=200): " << coast_weather.name << "\n";

    assert(north_weather.temperature_celsius < 5.0f);
    assert(south_weather.temperature_celsius > 15.0f);

    std::cout << "  ✓ Continental geography and static microclimates verified successfully.\n";
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " Realistic Medieval World Architecture Verification Suite\n";
    std::cout << "========================================================\n";

    test_tile_properties();
    test_weather_and_regions();
    test_spatial_grid_10k();
    test_continental_generation();

    print_memory_usage();

    std::cout << "========================================================\n";
    std::cout << " ALL TESTS PASSED SUCCESSFULLY!\n";
    std::cout << "========================================================\n";
    return 0;
}
