#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../weather.hpp"
#include "../region.hpp"
#include "../structure.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"
#include "../vision.hpp"
#include "../building_prefab.hpp"
#include "../action.hpp"
#include "../entity.hpp"
#include "../camera.hpp"

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <queue>
#include <utility>
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

    // Test vertical substrates
    auto open_air = Tile::getData(Tile::ID::OpenAir);
    CHECK(open_air.blocksMovement == true);
    CHECK(open_air.blocksSight == false);

    auto sub_rock = Tile::getData(Tile::ID::SubterraneanRock);
    CHECK(sub_rock.blocksMovement == true);
    CHECK(sub_rock.blocksSight == true);
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
    auto wood_floor = Structure::getData(Structure::ID::WoodFloor);
    CHECK(!wood_floor.blocksMovement);
    CHECK(!wood_floor.blocksSight);
    CHECK(wood_floor.glyph == '.');

    auto stone_floor = Structure::getData(Structure::ID::StoneFloor);
    CHECK(!stone_floor.blocksMovement);
    CHECK(!stone_floor.blocksSight);

    auto door_closed = Structure::getData(Structure::ID::DoorClosed);
    CHECK(door_closed.blocksMovement);
    CHECK(door_closed.blocksSight);
    CHECK(door_closed.glyph == '+');

    auto door_open = Structure::getData(Structure::ID::DoorOpen);
    CHECK(!door_open.blocksMovement);
    CHECK(!door_open.blocksSight);
    CHECK(door_open.glyph == '/');

    auto window = Structure::getData(Structure::ID::Window);
    CHECK(window.blocksMovement);
    CHECK(!window.blocksSight);
    CHECK(window.glyph == '"');

    auto stairs_down = Structure::getData(Structure::ID::StairsDown);
    CHECK(!stairs_down.blocksMovement);
    CHECK(stairs_down.glyph == '>');

    // 2. Validate JSON prefab catalog loading
    Architecture::PrefabCatalog catalog;
    bool loaded = catalog.load("data/prefabs/buildings.json");
    CHECK(loaded);
    CHECK(!catalog.get_all().empty());

    for (const auto &tmpl : catalog.get_all()) {
        CHECK(tmpl.is_valid());
        CHECK(tmpl.width > 0);
        CHECK(tmpl.height > 0);
    }

    // 3. Validate map stamping, door mechanics, & vegetation clearing
    Architecture::BuildingTemplate door_test;
    door_test.id = "test_cabin";
    door_test.name = "Test Cabin";
    door_test.width = 5;
    door_test.height = 5;
    door_test.wall_type = Structure::ID::WoodWall;
    door_test.floor_type = Structure::ID::WoodFloor;
    door_test.layout = {
        "#####",
        "#...#",
        "#...#",
        "#...#",
        "##+##"
    };

    GameMap::Map test_map(50, 50);
    for (int y = 10; y < 20; ++y) {
        for (int x = 10; x < 20; ++x) {
            test_map.set_vegetation(x, y, Vegetation::ID::DeciduousTree, 100);
        }
    }
    CHECK(test_map.has_vegetation(12, 12));

    // Stamp test cabin at (10, 10)
    bool stamped = test_map.stamp_building(10, 10, door_test, 0);
    CHECK(stamped);

    // Verify vegetation was cleared under footprint
    CHECK(!test_map.has_vegetation(10, 10));
    CHECK(!test_map.has_vegetation(12, 12));
    CHECK(!test_map.has_vegetation(12, 14));

    // Verify structures exist while base ground terrain is preserved!
    CHECK(test_map.at(10, 10) == Tile::ID::Grassland);
    CHECK(test_map.get_structure(10, 10).id == Structure::ID::WoodWall);
    CHECK(test_map.get_structure(12, 14).id == Structure::ID::DoorClosed);

    // 4. Validate Bump-to-Open Door mechanics & Line of Sight
    Engine::Entity player(12, 15, 0, '@', "Hero");
    Engine::Camera camera(30, 20);
    Vision::FOV fov(50, 50, 8);
    fov.compute(test_map, player.x, player.y, player.z);

    CHECK(!test_map.raycast_los(12, 15, 12, 12));
    CHECK(!fov.is_visible(12, 12));

    Engine::Action bump_door{Engine::ActionType::Move, 0, -1};
    auto result = Engine::ActionSystem::execute(bump_door, test_map, player, camera, fov);

    CHECK(result.success);
    CHECK(result.consumed_turn);
    CHECK(result.message == "You open the door.");
    CHECK(test_map.get_structure(12, 14).id == Structure::ID::DoorOpen);
    CHECK(test_map.at(12, 14) == Tile::ID::Grassland); // Ground preserved!
    CHECK(player.x == 12 && player.y == 15);

    CHECK(test_map.raycast_los(12, 15, 12, 12));
    CHECK(fov.is_visible(12, 12));

    auto step_in = Engine::ActionSystem::execute(bump_door, test_map, player, camera, fov);
    CHECK(step_in.success);
    CHECK(player.x == 12 && player.y == 14);

    // 5. Validate 90° rotation stamping
    bool rot_stamped = test_map.stamp_building(30, 10, door_test, 90);
    CHECK(rot_stamped);
    CHECK(test_map.at(30, 10) == Tile::ID::Grassland);
    CHECK(test_map.get_structure(30, 10).id == Structure::ID::WoodWall);
    CHECK(test_map.get_structure(30 + 5 - 1, 10 + 5 - 1).id == Structure::ID::WoodWall);
}

void test_ground_preservation_and_destruction() {
    GameMap::Map test_map(20, 20);
    test_map.set(5, 5, Tile::ID::ForestSoil);

    // Place a wooden wall over ForestSoil
    test_map.set_structure(5, 5, Structure::ID::WoodWall, 100);
    CHECK(test_map.at(5, 5) == Tile::ID::ForestSoil); // Ground preserved!
    CHECK(test_map.get_structure(5, 5).id == Structure::ID::WoodWall);
    CHECK(!test_map.is_walkable(5, 5));
    CHECK(test_map.blocks_sight(5, 5));

    // Damage wall by 40 HP
    bool destroyed = test_map.damage_structure(5, 5, 0, 40);
    CHECK(!destroyed);
    CHECK(test_map.get_structure(5, 5).durability == 60);
    CHECK(test_map.at(5, 5) == Tile::ID::ForestSoil);

    // Destroy wall by dealing 60 HP
    destroyed = test_map.damage_structure(5, 5, 0, 60);
    CHECK(destroyed);
    CHECK(test_map.get_structure(5, 5).id == Structure::ID::WoodScraps); // Leaves debris
    CHECK(test_map.at(5, 5) == Tile::ID::ForestSoil); // Ground still preserved!
    CHECK(test_map.is_walkable(5, 5)); // Rubble/scraps are traversable
    CHECK(!test_map.blocks_sight(5, 5));

    // Clear debris
    test_map.remove_structure(5, 5);
    CHECK(test_map.get_structure(5, 5).id == Structure::ID::None);
    CHECK(test_map.at(5, 5) == Tile::ID::ForestSoil);
}

void test_multi_level_and_stairs() {
    GameMap::Map test_map(25, 25);

    // Create a 2-level building template
    Architecture::BuildingTemplate tower;
    tower.id = "watchtower";
    tower.name = "Watchtower";
    tower.width = 5;
    tower.height = 5;

    Architecture::BuildingFloor fl0;
    fl0.level = 0;
    fl0.name = "Ground Floor";
    fl0.wall_type = Structure::ID::StoneWall;
    fl0.floor_type = Structure::ID::StoneFloor;
    fl0.layout = {
        "#####",
        "#...#",
        "#.<.#", // Stairs up to level 1 at local (2, 2) -> world (12, 12)
        "#...#",
        "##+##"
    };

    Architecture::BuildingFloor fl1;
    fl1.level = 1;
    fl1.name = "Upper Battlement";
    fl1.wall_type = Structure::ID::StoneWall;
    fl1.floor_type = Structure::ID::StoneFloor;
    fl1.layout = {
        "#####",
        "#...#",
        "#.>.#", // Stairs down to level 0 at local (2, 2) -> world (12, 12)
        "#...#",
        "#####"
    };

    tower.floors.push_back(fl0);
    tower.floors.push_back(fl1);

    bool stamped = test_map.stamp_building(10, 10, tower, 0);
    CHECK(stamped);

    // Verify level 0
    CHECK(test_map.get_structure(10, 10, 0).id == Structure::ID::StoneWall);
    CHECK(test_map.get_structure(12, 12, 0).id == Structure::ID::StairsUp);

    // Verify level 1
    CHECK(test_map.get_structure(10, 10, 1).id == Structure::ID::StoneWall);
    CHECK(test_map.get_structure(12, 12, 1).id == Structure::ID::StairsDown);

    // Verify outside upper level is OpenAir
    CHECK(test_map.at(5, 5, 1) == Tile::ID::OpenAir);
    CHECK(!test_map.is_walkable(5, 5, 1));
    CHECK(!test_map.blocks_sight(5, 5, 1));

    // Player ascends using ActionType::Interact ('e')
    Engine::Entity player(12, 12, 0, '@', "Hero");
    Engine::Camera camera(20, 20);
    Vision::FOV fov(25, 25, 6);
    fov.compute(test_map, player.x, player.y, player.z);

    Engine::Action interact{Engine::ActionType::Interact, 0, 0};
    auto res_ascend = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
    CHECK(res_ascend.success);
    CHECK(player.z == 1);
    CHECK(res_ascend.message.find("ascend") != std::string::npos);

    // Player descends back using ActionType::Interact ('e')
    auto res_descend = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
    CHECK(res_descend.success);
    CHECK(player.z == 0);
    CHECK(res_descend.message.find("descend") != std::string::npos);
}

void test_support_cascade_destruction() {
    GameMap::Map test_map(30, 30);

    // Build an isolated column wall on Z=0
    test_map.set_structure(15, 15, 0, Structure::ID::WoodWall, 100);

    // Build an upper floor tile resting directly on this column on Z=1
    test_map.set_structure(15, 15, 1, Structure::ID::WoodFloor, 100);

    CHECK(test_map.has_structural_support(15, 15, 1) == true);

    // Demolish the ground wall completely
    test_map.remove_structure(15, 15, 0);

    // Cascade destruction triggers:
    // Upper floor at (15, 15, 1) lost support and collapsed!
    CHECK(test_map.get_structure(15, 15, 1).id == Structure::ID::None);

    // Falling debris landed on (15, 15, 0)
    CHECK(test_map.get_structure(15, 15, 0).id == Structure::ID::WoodScraps);
}

namespace {

struct LayoutCoord {
    int x;
    int y;
};

bool is_traversable_glyph(char c) noexcept {
    return c == '.' || c == '+' || c == '/' || c == '<' || c == '>' || c == 'H';
}

std::vector<std::vector<bool>> compute_layout_reachability(
    const Architecture::BuildingFloor &floor,
    int width,
    int height,
    const std::vector<LayoutCoord> &starts)
{
    std::vector<std::vector<bool>> visited(height, std::vector<bool>(width, false));
    std::queue<LayoutCoord> q;
    for (const auto &s : starts) {
        if (s.x >= 0 && s.x < width && s.y >= 0 && s.y < height) {
            visited[s.y][s.x] = true;
            q.push(s);
        }
    }

    static const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
    while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();

        for (const auto &d : dirs) {
            int nx = cx + d[0];
            int ny = cy + d[1];
            if (nx >= 0 && nx < width && ny >= 0 && ny < height && !visited[ny][nx]) {
                char c = floor.layout[ny][nx];
                if (is_traversable_glyph(c)) {
                    visited[ny][nx] = true;
                    q.push(LayoutCoord{nx, ny});
                }
            }
        }
    }
    return visited;
}

} // namespace

void test_prefab_catalog_connectivity_and_layout_rules() {
    Architecture::PrefabCatalog catalog;
    bool loaded = catalog.load("data/prefabs/buildings.json");
    CHECK(loaded);
    CHECK(!catalog.get_all().empty());

    bool found_three_floor_building = false;

    for (const auto &tmpl : catalog.get_all()) {
        CHECK(tmpl.is_valid());
        CHECK(tmpl.width >= 3 && tmpl.height >= 3);
        CHECK(!tmpl.floors.empty());

        // 0. Verify all levels have identical dimensions (width x height) matching template
        for (const auto &fl : tmpl.floors) {
            if (static_cast<int>(fl.layout.size()) != tmpl.height) {
                std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' level " << fl.level
                          << " has height " << fl.layout.size() << ", expected template height "
                          << tmpl.height << "!\n";
            }
            CHECK(static_cast<int>(fl.layout.size()) == tmpl.height);

            for (int y = 0; y < tmpl.height; ++y) {
                if (static_cast<int>(fl.layout[y].size()) != tmpl.width) {
                    std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' level " << fl.level
                              << " row " << y << " has length " << fl.layout[y].size()
                              << ", expected template width " << tmpl.width << "!\n";
                }
                CHECK(static_cast<int>(fl.layout[y].size()) == tmpl.width);
            }
        }

        // Cross-level dimension equivalence verification across all floors
        for (size_t i = 1; i < tmpl.floors.size(); ++i) {
            const auto &f0 = tmpl.floors[0];
            const auto &fi = tmpl.floors[i];
            if (f0.layout.size() != fi.layout.size()) {
                std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' height mismatch: level "
                          << f0.level << " (" << f0.layout.size() << ") != level "
                          << fi.level << " (" << fi.layout.size() << ")!\n";
            }
            CHECK(f0.layout.size() == fi.layout.size());

            for (size_t r = 0; r < f0.layout.size(); ++r) {
                if (f0.layout[r].size() != fi.layout[r].size()) {
                    std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' row " << r
                              << " length mismatch: level " << f0.level << " (" << f0.layout[r].size()
                              << ") != level " << fi.level << " (" << fi.layout[r].size() << ")!\n";
                }
                CHECK(f0.layout[r].size() == fi.layout[r].size());
            }
        }

        // 1. Ground floor validation & exterior entry point
        const auto *ground = tmpl.get_floor(0);
        CHECK(ground != nullptr);

        std::vector<LayoutCoord> ground_entries;
        for (int y = 0; y < tmpl.height; ++y) {
            for (int x = 0; x < tmpl.width; ++x) {
                char c = ground->layout[y][x];
                if (c == '+' || c == '/') {
                    bool exterior = (x == 0 || x == tmpl.width - 1 || y == 0 || y == tmpl.height - 1);
                    if (!exterior) {
                        static const int dirs[4][2] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
                        for (const auto &d : dirs) {
                            int nx = x + d[0];
                            int ny = y + d[1];
                            if (nx < 0 || nx >= tmpl.width || ny < 0 || ny >= tmpl.height || ground->layout[ny][nx] == ' ') {
                                exterior = true;
                                break;
                            }
                        }
                    }
                    if (exterior) {
                        ground_entries.push_back({x, y});
                    }
                }
            }
        }
        CHECK(!ground_entries.empty()); // Every building must have at least one ground entrance

        // 2. Ground floor reachability from entry points
        auto ground_visited = compute_layout_reachability(*ground, tmpl.width, tmpl.height, ground_entries);
        for (int y = 0; y < tmpl.height; ++y) {
            for (int x = 0; x < tmpl.width; ++x) {
                char c = ground->layout[y][x];
                if (is_traversable_glyph(c)) {
                    if (!ground_visited[y][x]) {
                        std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' ground floor tile at ("
                                  << x << ", " << y << ") glyph '" << c << "' is not reachable from any exterior entry point!\n";
                    }
                    CHECK(ground_visited[y][x]); // All ground floor tiles, doors, and stairs must be reachable from entrance
                }
            }
        }

        // 3. Multi-floor vertical connectivity & stair pairing
        for (const auto &fl : tmpl.floors) {
            for (int y = 0; y < tmpl.height; ++y) {
                for (int x = 0; x < tmpl.width; ++x) {
                    char c = fl.layout[y][x];
                    if (c == '<') { // StairsUp
                        const auto *upper = tmpl.get_floor(fl.level + 1);
                        if (upper != nullptr) {
                            if (upper->layout[y][x] != '>') {
                                std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' StairsUp at level "
                                          << fl.level << " (" << x << ", " << y << ") has no matching StairsDown on level "
                                          << (fl.level + 1) << " (found '" << upper->layout[y][x] << "')!\n";
                            }
                            CHECK(upper->layout[y][x] == '>'); // Matching StairsDown on floor above at same (x, y)
                        }
                    } else if (c == '>') { // StairsDown
                        const auto *lower = tmpl.get_floor(fl.level - 1);
                        if (lower != nullptr) {
                            if (lower->layout[y][x] != '<') {
                                std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' StairsDown at level "
                                          << fl.level << " (" << x << ", " << y << ") has no matching StairsUp on level "
                                          << (fl.level - 1) << " (found '" << lower->layout[y][x] << "')!\n";
                            }
                            CHECK(lower->layout[y][x] == '<'); // Matching StairsUp on floor below at same (x, y)
                        }
                    }
                }
            }
        }

        // 4. Inter-floor stair existence and non-ground reachability
        if (tmpl.floors.size() > 1) {
            std::vector<int> levels;
            for (const auto &fl : tmpl.floors) {
                levels.push_back(fl.level);
            }
            std::sort(levels.begin(), levels.end());

            // Verify adjacent levels have connecting stairs
            for (size_t i = 0; i + 1 < levels.size(); ++i) {
                int z1 = levels[i];
                int z2 = levels[i + 1];
                if (z2 == z1 + 1) {
                    const auto *fl1 = tmpl.get_floor(z1);
                    const auto *fl2 = tmpl.get_floor(z2);
                    int stair_pairs = 0;
                    for (int y = 0; y < tmpl.height; ++y) {
                        for (int x = 0; x < tmpl.width; ++x) {
                            if (fl1->layout[y][x] == '<' && fl2->layout[y][x] == '>') {
                                ++stair_pairs;
                            }
                        }
                    }
                    if (stair_pairs == 0) {
                        std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' has no vertical stairs connecting level "
                                  << z1 << " and level " << z2 << "!\n";
                    }
                    CHECK(stair_pairs > 0); // Adjacent floors must have at least one paired staircase
                }
            }

            // Verify reachability on non-ground floors from stairs
            for (const auto &fl : tmpl.floors) {
                if (fl.level == 0) continue;

                std::vector<LayoutCoord> floor_stairs;
                for (int y = 0; y < tmpl.height; ++y) {
                    for (int x = 0; x < tmpl.width; ++x) {
                        char c = fl.layout[y][x];
                        if (c == '<' || c == '>') {
                            floor_stairs.push_back({x, y});
                        }
                    }
                }
                CHECK(!floor_stairs.empty()); // Non-ground floor must have stairs

                auto fl_visited = compute_layout_reachability(fl, tmpl.width, tmpl.height, floor_stairs);
                for (int y = 0; y < tmpl.height; ++y) {
                    for (int x = 0; x < tmpl.width; ++x) {
                        char c = fl.layout[y][x];
                        if (is_traversable_glyph(c)) {
                            if (!fl_visited[y][x]) {
                                std::cerr << "[LAYOUT ERROR] Template '" << tmpl.id << "' level " << fl.level
                                          << " tile at (" << x << ", " << y << ") glyph '" << c
                                          << "' is not reachable from floor stairs!\n";
                            }
                            CHECK(fl_visited[y][x]); // Every floor tile and door must be reachable from stairs
                        }
                    }
                }
            }
        }

        if (tmpl.floors.size() >= 3) {
            found_three_floor_building = true;
            // Verify it has subterranean, surface, and elevated levels
            bool has_cellar = false;
            bool has_ground = false;
            bool has_upper = false;
            for (const auto &fl : tmpl.floors) {
                if (fl.level < 0) has_cellar = true;
                if (fl.level == 0) has_ground = true;
                if (fl.level > 0) has_upper = true;
            }
            CHECK(has_cellar);
            CHECK(has_ground);
            CHECK(has_upper);
        }

        // 5. Dynamic Map Stamping & Player Navigation across levels
        GameMap::Map test_map(tmpl.width + 30, tmpl.height + 30);
        int stamp_x = 10;
        int stamp_y = 10;
        bool stamped = test_map.stamp_building(stamp_x, stamp_y, tmpl, 0);
        CHECK(stamped);

        // Verify ground preservation across stamped footprint
        for (int ty = 0; ty < tmpl.height; ++ty) {
            for (int tx = 0; tx < tmpl.width; ++tx) {
                CHECK(test_map.at(stamp_x + tx, stamp_y + ty, 0) != Tile::ID::OpenAir);
            }
        }

        // If template has stairs, test player interaction dynamically
        if (tmpl.floors.size() > 1) {
            Engine::Camera camera(test_map.get_width(), test_map.get_height());
            Vision::FOV fov(test_map.get_width(), test_map.get_height(), 8);
            Engine::Action interact{Engine::ActionType::Interact, 0, 0};

            for (const auto &fl : tmpl.floors) {
                for (int y = 0; y < tmpl.height; ++y) {
                    for (int x = 0; x < tmpl.width; ++x) {
                        char c = fl.layout[y][x];
                        if (c == '<' && tmpl.get_floor(fl.level + 1) != nullptr) {
                            // Test ascending
                            Engine::Entity player(stamp_x + x, stamp_y + y, fl.level, '@', "Hero");
                            fov.compute(test_map, player.x, player.y, player.z);
                            auto res = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
                            CHECK(res.success);
                            CHECK(player.z == fl.level + 1);
                            CHECK(test_map.get_structure(player.x, player.y, player.z).id == Structure::ID::StairsDown);

                            // Test descending back
                            auto res_back = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
                            CHECK(res_back.success);
                            CHECK(player.z == fl.level);
                            CHECK(test_map.get_structure(player.x, player.y, player.z).id == Structure::ID::StairsUp);
                        } else if (c == '>' && tmpl.get_floor(fl.level - 1) != nullptr) {
                            // Test descending
                            Engine::Entity player(stamp_x + x, stamp_y + y, fl.level, '@', "Hero");
                            fov.compute(test_map, player.x, player.y, player.z);
                            auto res = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
                            CHECK(res.success);
                            CHECK(player.z == fl.level - 1);
                            CHECK(test_map.get_structure(player.x, player.y, player.z).id == Structure::ID::StairsUp);

                            // Test ascending back
                            auto res_back = Engine::ActionSystem::execute(interact, test_map, player, camera, fov);
                            CHECK(res_back.success);
                            CHECK(player.z == fl.level);
                            CHECK(test_map.get_structure(player.x, player.y, player.z).id == Structure::ID::StairsDown);
                        }
                    }
                }
            }
        }

        // Test 90-degree rotated stamping
        GameMap::Map rot_map(tmpl.height + 30, tmpl.width + 30);
        bool rot_stamped = rot_map.stamp_building(stamp_x, stamp_y, tmpl, 90);
        CHECK(rot_stamped);
    }

    CHECK(found_three_floor_building); // Ensures we verified at least one 3-level building layout
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
    test_ground_preservation_and_destruction();
    test_multi_level_and_stairs();
    test_support_cascade_destruction();
    test_prefab_catalog_connectivity_and_layout_rules();

    std::cout << "All architecture tests passed.\n";
    return 0;
}
