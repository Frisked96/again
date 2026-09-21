#include "test_common.hpp"
#include "../game_map.hpp"
#include "../tile.hpp"
#include "../vegetation.hpp"
#include "../vision.hpp"

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

int main() {
    test_vision_system();
    std::cout << "All vision tests passed.\n";
    return 0;
}
