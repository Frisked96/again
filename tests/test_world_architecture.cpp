#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../weather.hpp"
#include "../region.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"
#include "../vision.hpp"

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
    std::cout << "[TEST] Validating realistic medieval ground tiles...\n";

    // Test water bodies
    auto deep_water = Tile::getData(Tile::ID::DeepWater);
    assert(deep_water.blocksMovement == true);
    assert(deep_water.movementCost > 100.0f);

    auto coastal_water = Tile::getData(Tile::ID::CoastalWater);
    assert(coastal_water.blocksMovement == false);
    assert(coastal_water.movementCost > 1.0f);

    // Test ground substrates
    auto forest_soil = Tile::getData(Tile::ID::ForestSoil);
    assert(forest_soil.blocksMovement == false);
    assert(forest_soil.blocksSight == false); // Pure ground substrate does not block sight!
    assert(forest_soil.movementCost > 1.0f);

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

    std::cout << "  ✓ All ground substrate physical properties verified successfully.\n";
}

void test_vegetation_properties() {
    std::cout << "[TEST] Validating decoupled vegetation & resource flora...\n";

    // Deciduous Oak
    auto oak = Vegetation::getData(Vegetation::ID::DeciduousTree);
    assert(oak.blocksSight == true);
    assert(oak.blocksMovement == false);
    assert(oak.resourceType == Vegetation::ResourceType::Timber);
    assert(oak.maxYield == 100);
    assert(Vegetation::can_grow(Vegetation::ID::DeciduousTree, 15.0f, 0.60f) == true);
    assert(Vegetation::can_grow(Vegetation::ID::DeciduousTree, -15.0f, 0.60f) == false); // Too cold

    // Coniferous Pine
    auto pine = Vegetation::getData(Vegetation::ID::ConiferousTree);
    assert(pine.blocksSight == true);
    assert(pine.resourceType == Vegetation::ResourceType::Softwood);
    assert(Vegetation::can_grow(Vegetation::ID::ConiferousTree, -5.0f, 0.50f) == true);

    // Berry Bush
    auto bush = Vegetation::getData(Vegetation::ID::BerryBush);
    assert(bush.blocksSight == false); // Small bushes do not block line of sight
    assert(bush.resourceType == Vegetation::ResourceType::WildBerries);

    // Tree Stump (post-felling)
    auto stump = Vegetation::getData(Vegetation::ID::TreeStump);
    assert(stump.blocksSight == false); // Felled tree stump does not block sight!
    assert(stump.glyph == 'o');

    std::cout << "  ✓ Vegetation data, resources, and climate tolerance verified successfully.\n";
}

void test_vegetation_harvesting() {
    std::cout << "[TEST] Validating vegetation harvesting and physical state transitions...\n";
    GameMap::Map map(100, 100);

    // 1. Plant an oak tree over forest soil
    map.set(10, 10, Tile::ID::ForestSoil);
    map.set_vegetation(10, 10, Vegetation::ID::DeciduousTree, 100);

    assert(map.has_vegetation(10, 10));
    assert(map.blocks_sight(10, 10) == true); // Standing tree blocks sight

    // 2. Partial harvest (e.g. chop 30 wood)
    auto res1 = map.harvest_vegetation(10, 10, 30);
    assert(res1.resource == Vegetation::ResourceType::Timber);
    assert(res1.amount_gathered == 30);
    assert(res1.depleted == false);
    assert(map.get_vegetation(10, 10).resource_amount == 70);
    assert(map.blocks_sight(10, 10) == true); // Still standing

    // 3. Complete harvest (felling the tree)
    auto res2 = map.harvest_vegetation(10, 10, 70);
    assert(res2.resource == Vegetation::ResourceType::Timber);
    assert(res2.amount_gathered == 70);
    assert(res2.depleted == true);

    // 4. Verify state transition to stump over the original forest soil substrate
    auto veg_cell = map.get_vegetation(10, 10);
    assert(veg_cell.id == Vegetation::ID::TreeStump);
    assert(map.at(10, 10) == Tile::ID::ForestSoil); // Underlying ground preserved!
    assert(map.blocks_sight(10, 10) == false);       // Line of sight restored through the clearing!

    std::cout << "  ✓ Harvesting, resource gathering, and stump transition verified successfully.\n";
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
    std::cout << "[TEST] Allocating 10,000 x 10,000 WorldMap (100,000,000 cells x 3 grids)...\n";
    auto t0 = std::chrono::high_resolution_clock::now();

    GameMap::Map world(10000, 10000);

    auto t1 = std::chrono::high_resolution_clock::now();
    double alloc_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    std::cout << "  ✓ 100M cell map (tile, region, and 2-byte vegetation grids) allocated in "
              << std::fixed << std::setprecision(2) << alloc_ms << " ms (~400 MB RAM).\n";

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

    // Test movement cost combining tile, vegetation, and weather
    world.set(100, 100, Tile::ID::ForestSoil);
    world.set_region(100, 100, World::RegionID::LowlandMeadow);
    world.set_vegetation(100, 100, Vegetation::ID::None);
    float cost_bare = world.get_movement_cost(100, 100);
    assert(cost_bare > 1.0f);

    // Add DenseScrub vegetation (cost mult 1.6)
    world.set_vegetation(100, 100, Vegetation::ID::DenseScrub, 60);
    float cost_scrub = world.get_movement_cost(100, 100);
    assert(cost_scrub > cost_bare * 1.5f);

    // Test Line of Sight (Raycasting) through clear ground
    world.set(500, 500, Tile::ID::Grassland);
    world.set(500, 501, Tile::ID::Grassland);
    world.set(500, 502, Tile::ID::Grassland);
    world.set(500, 503, Tile::ID::Grassland);
    world.set_vegetation(500, 500, Vegetation::ID::None);
    world.set_vegetation(500, 501, Vegetation::ID::None);
    world.set_vegetation(500, 502, Vegetation::ID::None);
    world.set_vegetation(500, 503, Vegetation::ID::None);
    world.set_region(500, 500, World::RegionID::LowlandMeadow); // Clear weather (visibility 60)
    assert(world.raycast_los(500, 500, 500, 503) == true);

    // Obstruct with a standing DeciduousTree on cell (500, 502)
    world.set_vegetation(500, 502, Vegetation::ID::DeciduousTree, 100);
    assert(world.raycast_los(500, 500, 500, 503) == false);

    // Harvest the tree down to a stump: sight is unblocked!
    world.harvest_vegetation(500, 502, 100);
    assert(world.get_vegetation(500, 502).id == Vegetation::ID::TreeStump);
    assert(world.raycast_los(500, 500, 500, 503) == true);

    // Weather visibility limit clipping
    world.set_region(500, 500, World::RegionID::GlacialCrown); // Blizzard max visibility = 4
    assert(world.raycast_los(500, 500, 500, 510) == false); // 10 tiles away exceeds visibility 4

    std::cout << "  ✓ Spatial methods, vegetation raycast LoS, and dynamic cost verified successfully.\n";
}

void test_continental_generation() {
    std::cout << "[TEST] Running continental medieval generator with decoupled flora...\n";
    GameMap::Map world(10000, 10000);

    auto t0 = std::chrono::high_resolution_clock::now();
    auto spawn = GameMap::MapGenerator::generate(world, 1337);
    auto t1 = std::chrono::high_resolution_clock::now();

    double gen_sec = std::chrono::duration<double>(t1 - t0).count();
    std::cout << "  ✓ Generated 100,000,000 cells (substrates + flora) in "
              << std::fixed << std::setprecision(3) << gen_sec << " seconds!\n";

    // Spawn point validation
    std::cout << "  Spawn point: (" << spawn.x << ", " << spawn.y << ")\n";
    assert(world.in_bounds(spawn.x, spawn.y));
    assert(world.is_walkable(spawn.x, spawn.y));

    auto spawn_tile = Tile::getData(world.at(spawn.x, spawn.y));
    auto spawn_veg = world.get_vegetation(spawn.x, spawn.y);
    auto spawn_region = World::getRegionData(world.get_region(spawn.x, spawn.y));
    auto spawn_weather = world.get_weather(spawn.x, spawn.y);

    std::cout << "  Spawn ground substrate: " << spawn_tile.name << "\n";
    if (spawn_veg.id != Vegetation::ID::None) {
        std::cout << "  Spawn flora: " << Vegetation::getData(spawn_veg.id).name
                  << " (Yield: " << static_cast<int>(spawn_veg.resource_amount) << "%)\n";
    } else {
        std::cout << "  Spawn flora: None (clear ground)\n";
    }
    std::cout << "  Spawn region: " << spawn_region.name << "\n";
    std::cout << "  Spawn static climate: " << spawn_weather.name << " ("
              << spawn_weather.temperature_celsius << "°C, "
              << spawn_weather.humidity_pct << "% humidity)\n";

    // Sample Taiga vs Weald vs Road
    auto road_y = world.get_height() / 2;
    // Road center should have no vegetation
    assert(world.get_vegetation(world.get_width() / 2, road_y).id == Vegetation::ID::None);

    std::cout << "  ✓ Continental geography, decoupled flora, and road clearance verified successfully.\n";
}

void test_vision_system() {
    std::cout << "[TEST] Validating decoupled Vision subsystem (LoS & FOV)...\n";
    GameMap::Map map(100, 100);

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            map.set(x, y, Tile::ID::Grassland);
            map.set_vegetation(x, y, Vegetation::ID::None);
        }
    }

    // Direct Vision::has_line_of_sight
    assert(Vision::has_line_of_sight(map, 2, 2, 2, 6) == true);

    // Place sight blocker
    map.set_vegetation(2, 4, Vegetation::ID::DeciduousTree, 100);
    assert(Vision::has_line_of_sight(map, 2, 2, 2, 6) == false);

    // Test Vision::FOV computation
    Vision::FOV fov(100, 100, 5);
    fov.compute(map, 2, 2);

    assert(fov.is_visible(2, 2) == true);
    assert(fov.is_explored(2, 2) == true);
    assert(fov.is_visible(2, 3) == true);
    assert(fov.is_visible(2, 4) == true);  // The blocker itself is visible
    assert(fov.is_visible(2, 5) == false); // Behind the blocker is in shadow!

    std::cout << "  ✓ Decoupled Vision::has_line_of_sight and Vision::FOV verified successfully.\n";
}

int main() {
    std::cout << "========================================================\n";
    std::cout << " Decoupled Vegetation & Resource Architecture Test Suite\n";
    std::cout << "========================================================\n";

    test_tile_properties();
    test_vegetation_properties();
    test_vegetation_harvesting();
    test_weather_and_regions();
    test_spatial_grid_10k();
    test_continental_generation();
    test_vision_system();

    print_memory_usage();

    std::cout << "========================================================\n";
    std::cout << " ALL TESTS PASSED SUCCESSFULLY!\n";
    std::cout << "========================================================\n";
    return 0;
}
