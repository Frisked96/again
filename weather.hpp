#pragma once
#include <cstdint>
#include <string_view>

namespace Climate {

enum class WeatherID : uint8_t {
    ClearTemperate = 0,
    Overcast,
    LightDrizzle,
    HeavyDownpour,
    Thunderstorm,
    DenseMist,
    ScorchingArid,
    DustHaze,
    CrispFrost,
    LightFlurries,
    Blizzard,
    AlpineGale,
    Count
};

struct WeatherData {
    std::string_view name;
    std::string_view description;
    float temperature_celsius;
    float humidity_pct;
    float precipitation_mm_h;
    float wind_speed_kmh;
    int visibility_limit;       // Max tile distance for clear sight
    float movement_cost_mult;   // Traversal friction modifier
};

inline constexpr WeatherData getWeatherData(WeatherID id) noexcept {
    switch (id) {
    case WeatherID::ClearTemperate:
        return {
            "Clear Mild",
            "Pleasant, unobstructed skies with gentle air.",
            18.0f, 50.0f, 0.0f, 10.0f, 60, 1.0f
        };
    case WeatherID::Overcast:
        return {
            "Overcast",
            "A dense blanket of grey stratocumulus clouds.",
            14.0f, 75.0f, 0.0f, 15.0f, 35, 1.0f
        };
    case WeatherID::LightDrizzle:
        return {
            "Autumn Drizzle",
            "A gentle, persistent drizzle soaking the earth.",
            11.0f, 90.0f, 1.5f, 12.0f, 25, 1.05f
        };
    case WeatherID::HeavyDownpour:
        return {
            "Torrential Rain",
            "Driving sheets of heavy rain churn soil into deep mud.",
            13.0f, 98.0f, 15.0f, 45.0f, 14, 1.25f
        };
    case WeatherID::Thunderstorm:
        return {
            "Tempest",
            "Violent squalls, deafening thunder, and driving deluge.",
            15.0f, 95.0f, 30.0f, 65.0f, 8, 1.4f
        };
    case WeatherID::DenseMist:
        return {
            "Dense Mist",
            "Cold, stagnant fog clinging to low ground and mire.",
            8.0f, 100.0f, 0.2f, 3.0f, 5, 1.1f
        };
    case WeatherID::ScorchingArid:
        return {
            "Scorching Sun",
            "Blistering, dry heat radiating off sun-baked earth.",
            38.0f, 12.0f, 0.0f, 18.0f, 45, 1.15f
        };
    case WeatherID::DustHaze:
        return {
            "Sirocco Dust",
            "Choking arid wind carrying fine sand and silt.",
            34.0f, 18.0f, 0.0f, 50.0f, 10, 1.2f
        };
    case WeatherID::CrispFrost:
        return {
            "Crisp Frost",
            "Clear, biting subzero air; frost crunches underfoot.",
            -2.0f, 40.0f, 0.0f, 10.0f, 55, 1.0f
        };
    case WeatherID::LightFlurries:
        return {
            "Snow Flurries",
            "Soft flurries of powder snow drifting on cold gusts.",
            -4.0f, 80.0f, 1.0f, 15.0f, 20, 1.1f
        };
    case WeatherID::Blizzard:
        return {
            "Whiteout Blizzard",
            "Howling subpolar gale and blinding, wind-whipped snow.",
            -15.0f, 92.0f, 12.0f, 75.0f, 4, 1.6f
        };
    case WeatherID::AlpineGale:
        return {
            "Alpine Gale",
            "Relentless freezing summit winds whistling across barren rock.",
            -10.0f, 60.0f, 0.5f, 90.0f, 12, 1.35f
        };
    default:
        return {
            "Unknown Weather",
            "Uncharted atmospheric condition.",
            15.0f, 50.0f, 0.0f, 0.0f, 30, 1.0f
        };
    }
}

} // namespace Climate
