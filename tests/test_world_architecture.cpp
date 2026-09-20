#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../weather.hpp"
#include "../region.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"
#include "../vision.hpp"
#include "../building_prefab.hpp"
#include "../action.hpp"
#include "../entity.hpp"
#include "../camera.hpp"

#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <vector>

#define CHECK(cond) \
  do { \
    if (!(cond)) { \
      std::cerr << "[TEST FAILED] " << __FILE__ << ":" << __LINE__ << " in " << __func__ \
                << "(): assertion failed: (" #cond ")\n"; \
      std::exit(1); \
    } \
  } while (0)

void test_tile_properties() {
    // Test water bodies
    auto deep_water = Tile::getData(Tile::ID::DeepWater);
    CHECK(deep_water.blocksMovement == true);
    CHECK(deep_water.movementCost > 100.0f);

    auto coastal_water = Tile::getData(Tile::ID::CoastalWater);
    CHECK(coastal_water.blocksMovement == false);
    CHECK(coastal_water.movementCost > 1.0f);

    // Test ground substrates
    auto forest_soil = Tile::getData(Tile::ID::ForestSoil);
    CHECK(forest_soil.blocksMovement == false);
    CHECK(forest_soil.blocksSight == false);
    CHECK(forest_soil.movementCost > 1.0f);

    auto grassland = Tile::getData(Tile::ID::Grassland);
    CHECK(grassland.blocksMovement == false);
    CHECK(grassland.blocksSight == false);
    CHECK(grassland.movementCost == 1.0f);

    // Test arid / desert
    auto sand = Tile::getData(Tile::ID::DesertSand);
    CHECK(sand.blocksMovement == false);
    CHECK(sand.movementCost > 1.0f);

    // Test snow & ice
    auto snow_deep = Tile::getData(Tile::ID::SnowDeep);
    CHECK(snow_deep.blocksMovement == false);
    CHECK(snow_deep.movementCost > 2.0f);

    // Test alpine crags
    auto summit = Tile::getData(Tile::ID::MountainPeak);
    CHECK(summit.blocksMovement == true);
    CHECK(summit.blocksSight == true);
}

void test_vegetation_properties() {
    // Deciduous Oak
    auto oak = Vegetation::getData(Vegetation::ID::DeciduousTree);
    CHECK(oak.blocksSight == true);
    CHECK(oak.blocksMovement == false);
    CHECK(oak.resourceType == Vegetation::ResourceType::Timber);
    CHECK(oak.maxYield == 100);
    CHECK(Vegetation::can_grow(Vegetation::ID::DeciduousTree, 15.0f, 0.60f) == true);
    CHECK(Vegetation::can_grow(Vegetation::ID::DeciduousTree, -15.0f, 0.60f) == false);

    // Coniferous Pine
    auto pine = Vegetation::getData(Vegetation::ID::ConiferousTree);
    CHECK(pine.blocksSight == true);
    CHECK(pine.resourceType == Vegetation::ResourceType::Softwood);
    CHECK(Vegetation::can_grow(Vegetation::ID::ConiferousTree, -5.0f, 0.50f) == true);

    // Berry Bush
    auto bush = Vegetation::getData(Vegetation::ID::BerryBush);
    CHECK(bush.blocksSight == false);
    CHECK(bush.resourceType == Vegetation::ResourceType::WildBerries);

    // Tree Stump (post-felling)
    auto stump = Vegetation::getData(Vegetation::ID::TreeStump);
    CHECK(stump.blocksSight == false);
    CHECK(stump.glyph == 'o');
}

void test_vegetation_harvesting() {
    GameMap::Map map(100, 100);

    // 1. Plant an oak tree over forest soil
    map.set(10, 10, Tile::ID::ForestSoil);
    map.set_vegetation(10, 10, Vegetation::ID::DeciduousTree, 100);

    CHECK(map.has_vegetation(10, 10));
    CHECK(map.blocks_sight(10, 10) == true);

    // 2. Partial harvest (e.g. chop 30 wood)
    auto res1 = map.harvest_vegetation(10, 10, 30);
    CHECK(res1.resource == Vegetation::ResourceType::Timber);
    CHECK(res1.amount_gathered == 30);
    CHECK(res1.depleted == false);
    CHECK(map.get_vegetation(10, 10).resource_amount == 70);
    CHECK(map.blocks_sight(10, 10) == true);

    // 3. Complete harvest (felling the tree)
    auto res2 = map.harvest_vegetation(10, 10, 70);
    CHECK(res2.resource == Vegetation::ResourceType::Timber);
    CHECK(res2.amount_gathered == 70);
    CHECK(res2.depleted == true);

    // 4. Verify state transition to stump over the original forest soil substrate
    auto veg_cell = map.get_vegetation(10, 10);
    CHECK(veg_cell.id == Vegetation::ID::TreeStump);
    CHECK(map.at(10, 10) == Tile::ID::ForestSoil);
    CHECK(map.blocks_sight(10, 10) == false);
}

void test_weather_and_regions() {
    auto scorching = Climate::getWeatherData(Climate::WeatherID::ScorchingArid);
    CHECK(scorching.temperature_celsius > 30.0f);
    CHECK(scorching.humidity_pct < 20.0f);

    auto blizzard = Climate::getWeatherData(Climate::WeatherID::Blizzard);
    CHECK(blizzard.temperature_celsius < -10.0f);
    CHECK(blizzard.visibility_limit <= 5);
    CHECK(blizzard.movement_cost_mult > 1.5f);

    auto mist = Climate::getWeatherData(Climate::WeatherID::DenseMist);
    CHECK(mist.humidity_pct == 100.0f);
    CHECK(mist.visibility_limit <= 6);

    auto waste_region = World::getRegionData(World::RegionID::AridWaste);
    CHECK(waste_region.default_weather == Climate::WeatherID::ScorchingArid);

    auto glacial_region = World::getRegionData(World::RegionID::GlacialCrown);
    CHECK(glacial_region.default_weather == Climate::WeatherID::Blizzard);

    auto fen_region = World::getRegionData(World::RegionID::PeatFen);
    CHECK(fen_region.default_weather == Climate::WeatherID::DenseMist);
}

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

void test_vision_system() {
    GameMap::Map map(100, 100);

    for (int y = 0; y < 10; ++y) {
        for (int x = 0; x < 10; ++x) {
            map.set(x, y, Tile::ID::Grassland);
            map.set_vegetation(x, y, Vegetation::ID::None);
        }
    }

    // Direct Vision::has_line_of_sight
    CHECK(Vision::has_line_of_sight(map, 2, 2, 2, 6) == true);

    // Place sight blocker
    map.set_vegetation(2, 4, Vegetation::ID::DeciduousTree, 100);
    CHECK(Vision::has_line_of_sight(map, 2, 2, 2, 6) == false);

    // Test Vision::FOV computation
    Vision::FOV fov(100, 100, 5);
    fov.compute(map, 2, 2);

    CHECK(fov.is_visible(2, 2) == true);
    CHECK(fov.is_explored(2, 2) == true);
    CHECK(fov.is_visible(2, 3) == true);
    CHECK(fov.is_visible(2, 4) == true);
    CHECK(fov.is_visible(2, 5) == false);
}

void test_pregenerated_climate_gradient() {
    GameMap::Map world(10000, 10000);
    GameMap::MapGenerator::generate(world, 1337);

    CHECK(world.has_climate_gradient() == true);

    // 1. Validate North-to-South continental temperature progression
    float north_temp = world.get_temperature(5000, 200);
    float mid_temp = world.get_temperature(5000, 5000);
    float south_temp = world.get_temperature(5000, 9800);

    CHECK(north_temp < mid_temp);
    CHECK(mid_temp < south_temp);
    CHECK(north_temp < 0.0f);
    CHECK(south_temp > 25.0f);

    // Validate southern desert peak
    float peak_desert_temp = -100.0f;
    for (int x = 500; x < 9500; x += 500) {
        for (int y = 8500; y < 9950; y += 200) {
            float t = world.get_temperature(x, y);
            if (t > peak_desert_temp) {
                peak_desert_temp = t;
            }
        }
    }
    CHECK(peak_desert_temp >= 38.0f && peak_desert_temp <= 42.0f);

    // Validate elevation lapse rate
    float valley_temp = world.get_temperature(1000, 5000);
    CHECK(mid_temp < valley_temp);

    // 2. Validate tile-by-tile smooth continuity
    float max_step_delta = 0.0f;
    for (int y = 4500; y < 5500; ++y) {
        float t1 = world.get_temperature(5000, y);
        float t2 = world.get_temperature(5000, y + 1);
        float dt = std::abs(t2 - t1);
        if (dt > max_step_delta) {
            max_step_delta = dt;
        }
        CHECK(dt < 0.1f);
    }
    CHECK(max_step_delta < 0.05f);

    // 3. Validate procedural weather diversity
    std::vector<std::string_view> observed_weathers;
    for (int sample_y = 500; sample_y < 9500; sample_y += 1500) {
        for (int sample_x = 500; sample_x < 9500; sample_x += 1500) {
            auto sw = world.get_weather(sample_x, sample_y);
            bool found = false;
            for (auto name : observed_weathers) {
                if (name == sw.name) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                observed_weathers.push_back(sw.name);
            }
        }
    }
    CHECK(observed_weathers.size() >= 4);

    // 4. Validate get_weather integrates pre-generated gradient
    auto w = world.get_weather(5000, 5000);
    CHECK(std::abs(w.temperature_celsius - mid_temp) < 0.0001f);
    CHECK(w.humidity_pct >= 5.0f && w.humidity_pct <= 100.0f);
}

void test_building_prefabs_and_doors() {
    // 1. Validate structural tiles
    auto wood_floor = Tile::getData(Tile::ID::WoodFloor);
    CHECK(!wood_floor.blocksMovement);
    CHECK(!wood_floor.blocksSight);
    CHECK(wood_floor.glyph == '.');

    auto stone_floor = Tile::getData(Tile::ID::StoneFloor);
    CHECK(!stone_floor.blocksMovement);
    CHECK(!stone_floor.blocksSight);

    auto door_closed = Tile::getData(Tile::ID::DoorClosed);
    CHECK(door_closed.blocksMovement);
    CHECK(door_closed.blocksSight);
    CHECK(door_closed.glyph == '+');

    auto door_open = Tile::getData(Tile::ID::DoorOpen);
    CHECK(!door_open.blocksMovement);
    CHECK(!door_open.blocksSight);
    CHECK(door_open.glyph == '/');

    auto window = Tile::getData(Tile::ID::Window);
    CHECK(window.blocksMovement);
    CHECK(!window.blocksSight);
    CHECK(window.glyph == '"');

    auto stairs_down = Tile::getData(Tile::ID::StairsDown);
    CHECK(!stairs_down.blocksMovement);
    CHECK(stairs_down.glyph == '>');

    // 2. Validate JSON prefab catalog loading
    Architecture::PrefabCatalog catalog;
    bool loaded = catalog.load("data/prefabs/buildings.json");
    CHECK(loaded);
    CHECK(catalog.size() >= 6);

    auto smithy = catalog.find_by_id("blacksmith_smithy");
    CHECK(smithy != nullptr);
    CHECK(smithy->width == 11);
    CHECK(smithy->height == 9);
    CHECK(smithy->rooms.size() == 2);
    CHECK(smithy->get_room_name(2, 2) == "Forge & Anvil Court");
    CHECK(smithy->get_room_name(8, 2) == "Smith's Quarters");

    auto hovel = catalog.find_by_id("peasant_hovel");
    CHECK(hovel != nullptr);
    CHECK(hovel->width == 6);
    CHECK(hovel->height == 6);

    // 3. Validate map stamping & vegetation clearing
    GameMap::Map test_map(50, 50);
    for (int y = 10; y < 25; ++y) {
        for (int x = 10; x < 25; ++x) {
            test_map.set_vegetation(x, y, Vegetation::ID::DeciduousTree, 100);
        }
    }
    CHECK(test_map.has_vegetation(15, 15));

    // Stamp blacksmith at (10, 10)
    bool stamped = test_map.stamp_building(10, 10, *smithy, 0);
    CHECK(stamped);

    // Verify vegetation was cleared under footprint
    CHECK(!test_map.has_vegetation(10, 10));
    CHECK(!test_map.has_vegetation(15, 15));
    CHECK(!test_map.has_vegetation(20, 18));

    // Verify walls, floors, doors, and furniture
    CHECK(test_map.at(10, 10) == Tile::ID::StoneWall);
    CHECK(test_map.at(13, 12) == Tile::ID::Anvil);
    CHECK(test_map.at(15, 18) == Tile::ID::DoorClosed);

    // 4. Validate Bump-to-Open Door mechanics & Line of Sight
    Engine::Entity player(15, 19, '@', "Hero");
    Engine::Camera camera(30, 20);
    Vision::FOV fov(50, 50, 8);
    fov.compute(test_map, player.x, player.y);

    CHECK(!test_map.raycast_los(15, 19, 15, 15));
    CHECK(!fov.is_visible(15, 15));

    Engine::Action bump_door{Engine::ActionType::Move, 0, -1};
    auto result = Engine::ActionSystem::execute(bump_door, test_map, player, camera, fov);

    CHECK(result.success);
    CHECK(result.consumed_turn);
    CHECK(result.message == "You open the door.");
    CHECK(test_map.at(15, 18) == Tile::ID::DoorOpen);
    CHECK(player.x == 15 && player.y == 19);

    CHECK(test_map.raycast_los(15, 19, 15, 15));
    CHECK(fov.is_visible(15, 15));

    auto step_in = Engine::ActionSystem::execute(bump_door, test_map, player, camera, fov);
    CHECK(step_in.success);
    CHECK(player.x == 15 && player.y == 18);

    // 5. Validate 90° rotation stamping
    bool rot_stamped = test_map.stamp_building(30, 10, *smithy, 90);
    CHECK(rot_stamped);
    CHECK(test_map.at(30, 10) == Tile::ID::StoneWall);
    CHECK(test_map.at(30 + 9 - 1, 10 + 11 - 1) == Tile::ID::StoneWall);
}

int main() {
    test_tile_properties();
    test_vegetation_properties();
    test_vegetation_harvesting();
    test_weather_and_regions();
    test_spatial_grid_10k();
    test_continental_generation();
    test_vision_system();
    test_pregenerated_climate_gradient();
    test_building_prefabs_and_doors();

    std::cout << "All architecture tests passed.\n";
    return 0;
}
