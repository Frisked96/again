#pragma once
#include "weather.hpp"
#include <cstdint>
#include <string_view>

namespace World {

enum class RegionID : uint8_t {
    AridWaste = 0,
    DrySteppe,
    LowlandMeadow,
    DeciduousWeald,
    PeatFen,
    BorealTaiga,
    FrostTundra,
    HighlandPeaks,
    GlacialCrown,
    CoastalCliffs,
    Count
};

struct RegionData {
    std::string_view name;
    std::string_view description;
    Climate::WeatherID default_weather;
    float base_elevation; // Normalized 0.0 - 1.0
};

inline constexpr RegionData getRegionData(RegionID id) noexcept {
    switch (id) {
    case RegionID::AridWaste:
        return {
            "The Sunken Wastes",
            "An expanse of barren shifting dunes and sun-baked clay flats.",
            Climate::WeatherID::ScorchingArid,
            0.15f
        };
    case RegionID::DrySteppe:
        return {
            "The Great Steppe",
            "Vast, windblown semi-arid shortgrass plains under dusty horizons.",
            Climate::WeatherID::DustHaze,
            0.25f
        };
    case RegionID::LowlandMeadow:
        return {
            "Lowland Meadows",
            "Fertile, rolling pastures and river-fed loamy valleys.",
            Climate::WeatherID::ClearTemperate,
            0.20f
        };
    case RegionID::DeciduousWeald:
        return {
            "The Oldwood Weald",
            "Ancient temperate broadleaf forest of dense oak, birch, and thick underbrush.",
            Climate::WeatherID::Overcast,
            0.35f
        };
    case RegionID::PeatFen:
        return {
            "Blackwater Fen",
            "Treacherous quaking mires, stagnant pools, and perpetual heavy mist.",
            Climate::WeatherID::DenseMist,
            0.10f
        };
    case RegionID::BorealTaiga:
        return {
            "Boreal Taiga",
            "Vast, somber evergreen forests of spruce and pine dusted in frost.",
            Climate::WeatherID::LightFlurries,
            0.45f
        };
    case RegionID::FrostTundra:
        return {
            "The Frost Tundra",
            "Barren, wind-scoured permafrost plains where only moss and lichen cling.",
            Climate::WeatherID::CrispFrost,
            0.40f
        };
    case RegionID::HighlandPeaks:
        return {
            "The Crags",
            "Towering granite precipices, razor ridges, and relentless mountain gales.",
            Climate::WeatherID::AlpineGale,
            0.85f
        };
    case RegionID::GlacialCrown:
        return {
            "The Glacial Crown",
            "Ancient sheet ice, deep crevasses, and eternal whiteout blizzards.",
            Climate::WeatherID::Blizzard,
            0.95f
        };
    case RegionID::CoastalCliffs:
        return {
            "The Storm Coast",
            "Jagged sea cliffs and churning surf battered by torrential rain.",
            Climate::WeatherID::HeavyDownpour,
            0.05f
        };
    default:
        return {
            "Uncharted Wilderness",
            "Undocumented region of the realm.",
            Climate::WeatherID::ClearTemperate,
            0.2f
        };
    }
}

} // namespace World
