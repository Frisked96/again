#include "test_common.hpp"
#include "../game_map.hpp"
#include "../map_generator.hpp"
#include "../region.hpp"
#include "../weather.hpp"

#include <cmath>
#include <string_view>
#include <vector>

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

int main() {
    test_weather_and_regions();
    test_pregenerated_climate_gradient();
    std::cout << "All weather and region tests passed.\n";
    return 0;
}
