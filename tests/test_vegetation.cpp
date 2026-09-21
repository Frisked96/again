#include "test_common.hpp"
#include "../game_map.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"

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

int main() {
    test_vegetation_properties();
    test_vegetation_harvesting();
    std::cout << "All vegetation tests passed.\n";
    return 0;
}
