#include "test_common.hpp"
#include "../tile.hpp"

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

int main() {
    test_tile_properties();
    std::cout << "All tile tests passed.\n";
    return 0;
}
