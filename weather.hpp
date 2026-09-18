#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string_view>
#include <vector>

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

class GradientMap {
public:
    static constexpr int MACRO_DIM = 32;

private:
    int width_{0};
    int height_{0};
    std::vector<float> macro_elev_;
    std::vector<float> macro_moist_;
    std::vector<float> macro_temp_;
    bool initialized_{false};

    static float smoothstep(float t) noexcept {
        return t * t * (3.0f - 2.0f * t);
    }

    float sample_grid(const std::vector<float> &grid, int x, int y) const noexcept {
        if (grid.size() < MACRO_DIM * MACRO_DIM || width_ <= 1 || height_ <= 1) {
            return 0.5f;
        }
        const float step_x = static_cast<float>(MACRO_DIM - 1) / static_cast<float>(width_ - 1);
        const float step_y = static_cast<float>(MACRO_DIM - 1) / static_cast<float>(height_ - 1);

        float gx = static_cast<float>(std::clamp(x, 0, width_ - 1)) * step_x;
        float gy = static_cast<float>(std::clamp(y, 0, height_ - 1)) * step_y;

        int mx0 = static_cast<int>(gx);
        int mx1 = std::min(MACRO_DIM - 1, mx0 + 1);
        float tx = smoothstep(gx - static_cast<float>(mx0));

        int my0 = static_cast<int>(gy);
        int my1 = std::min(MACRO_DIM - 1, my0 + 1);
        float ty = smoothstep(gy - static_cast<float>(my0));

        float v00 = grid[my0 * MACRO_DIM + mx0];
        float v10 = grid[my0 * MACRO_DIM + mx1];
        float v01 = grid[my1 * MACRO_DIM + mx0];
        float v11 = grid[my1 * MACRO_DIM + mx1];

        return (1.0f - ty) * ((1.0f - tx) * v00 + tx * v10) +
               ty * ((1.0f - tx) * v01 + tx * v11);
    }

public:
    GradientMap() = default;

    void init(int w, int h, std::vector<float> elev, std::vector<float> moist, std::vector<float> temp) {
        width_ = w;
        height_ = h;
        macro_elev_ = std::move(elev);
        macro_moist_ = std::move(moist);
        macro_temp_ = std::move(temp);
        initialized_ = (macro_elev_.size() >= MACRO_DIM * MACRO_DIM &&
                        macro_moist_.size() >= MACRO_DIM * MACRO_DIM &&
                        width_ > 0 && height_ > 0);
    }

    [[nodiscard]] bool is_initialized() const noexcept { return initialized_; }

    [[nodiscard]] float sample_elevation(int x, int y) const noexcept {
        return sample_grid(macro_elev_, x, y);
    }

    [[nodiscard]] float sample_moisture(int x, int y) const noexcept {
        return sample_grid(macro_moist_, x, y);
    }

    [[nodiscard]] float sample_macro_temp(int x, int y) const noexcept {
        if (macro_temp_.empty()) {
            return 0.5f;
        }
        return sample_grid(macro_temp_, x, y);
    }

    // Unified continental climate model: computes effective temperature [0.0 - 1.0]
    static float compute_effective_temp(float y_norm, float elev, float temp_noise) noexcept {
        // Base latitude progression: North (cold, y=0) to South (hot, y=1)
        float base_lat = 0.05f + 0.85f * y_norm;
        // Procedural thermal variance from world seed (+/- 0.08)
        float thermal_anomaly = (temp_noise - 0.5f) * 0.16f;
        // Elevation lapse rate: summits cool down by up to 0.40
        float lapse = 0.40f * elev;
        // Lowland southern heat dome: sun-baked desert basins trap arid heat to reach ~40°C
        float heat_basin = 0.0f;
        if (y_norm > 0.65f && elev < 0.40f) {
            float south_factor = (y_norm - 0.65f) / 0.35f;
            float low_factor = 1.0f - (elev / 0.40f);
            heat_basin = 0.16f * south_factor * low_factor;
        }
        return std::clamp(base_lat + thermal_anomaly - lapse + heat_basin, 0.0f, 1.0f);
    }

    [[nodiscard]] float sample_effective_temperature(int x, int y) const noexcept {
        if (!initialized_ || height_ <= 0) {
            return 0.5f;
        }
        float y_norm = static_cast<float>(std::clamp(y, 0, height_ - 1)) / static_cast<float>(height_);
        float elev = sample_elevation(x, y);
        float temp_noise = sample_macro_temp(x, y);
        return compute_effective_temp(y_norm, elev, temp_noise);
    }

    [[nodiscard]] float sample_temperature_celsius(int x, int y) const noexcept {
        float t = sample_effective_temperature(x, y);
        // Calibrated continuous climate response across continental Whittaker biomes:
        // Polar/Taiga (0.00 - 0.25): -18.0°C to +1.0°C
        // Temperate   (0.25 - 0.65):  +1.0°C to +24.0°C
        // Arid/Desert (0.65 - 1.00): +24.0°C to +41.5°C (peaking around 40°C - 41.5°C in deep desert basins)
        if (t <= 0.25f) {
            return -18.0f + (t / 0.25f) * 19.0f;
        } else if (t <= 0.65f) {
            return 1.0f + ((t - 0.25f) / 0.40f) * 23.0f;
        } else {
            float desert_ratio = std::clamp((t - 0.65f) / 0.35f, 0.0f, 1.0f);
            return 24.0f + desert_ratio * 17.5f;
        }
    }

    [[nodiscard]] float sample_humidity_pct(int x, int y) const noexcept {
        float moist = sample_moisture(x, y);
        // Continuous moisture mapping from arid desert (10%) to fen/wetland (98%)
        return std::clamp(10.0f + moist * 88.0f, 5.0f, 100.0f);
    }

    // Procedural randomized atmospheric weather condition generation
    [[nodiscard]] WeatherID sample_weather_id(int x, int y) const noexcept {
        float temp = sample_temperature_celsius(x, y);
        float moist = sample_moisture(x, y);
        float elev = sample_elevation(x, y);
        float noise = sample_macro_temp(x, y);

        // Subzero climates (< 0°C)
        if (temp < 0.0f) {
            if (elev >= 0.70f) {
                return WeatherID::AlpineGale;
            }
            if (temp < -10.0f || (noise > 0.60f && moist > 0.45f)) {
                return WeatherID::Blizzard;
            }
            if (moist > 0.35f) {
                return WeatherID::LightFlurries;
            }
            return WeatherID::CrispFrost;
        }

        // Hot / Arid climates (>= 28°C)
        if (temp >= 28.0f) {
            if (temp >= 36.0f) {
                return WeatherID::ScorchingArid;
            }
            if (moist < 0.45f || noise > 0.50f) {
                return WeatherID::DustHaze;
            }
            return WeatherID::ClearTemperate;
        }

        // Cold to Mild (0°C to 14°C)
        if (temp < 14.0f) {
            if (moist > 0.75f && noise < 0.65f) {
                return WeatherID::DenseMist;
            }
            if (moist > 0.70f && noise >= 0.75f) {
                return WeatherID::Thunderstorm;
            }
            if (moist > 0.58f) {
                return WeatherID::LightDrizzle;
            }
            if (moist > 0.40f || noise > 0.45f) {
                return WeatherID::Overcast;
            }
            return WeatherID::ClearTemperate;
        }

        // Temperate (14°C to 28°C)
        if (moist > 0.80f && noise >= 0.75f) {
            return WeatherID::Thunderstorm;
        }
        if (moist > 0.68f) {
            return (noise > 0.50f) ? WeatherID::HeavyDownpour : WeatherID::LightDrizzle;
        }
        if (moist > 0.50f && noise > 0.48f) {
            return WeatherID::Overcast;
        }

        // Default clear mild
        return WeatherID::ClearTemperate;
    }
};

} // namespace Climate
