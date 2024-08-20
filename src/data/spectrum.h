#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>


class spectrum {
public:
    static constexpr int NUM_SAMPLES = 81; // 400nm to 800nm with 5nm steps
    static constexpr float START_WAVELENGTH = 400.0f;
    static constexpr float END_WAVELENGTH = 800.0f;

    spectrum() : data_(NUM_SAMPLES, 0.0f) {}
    explicit spectrum(float value) : data_(NUM_SAMPLES, value) {}
    spectrum(const float* values, int count) : data_(values, values + std::min(count, NUM_SAMPLES)) {}

    float& operator[](int index) { return data_[index]; }
    const float& operator[](int index) const { return data_[index]; }

    // RGB to Spectrum conversion
    static spectrum from_rgb(float r, float g, float b) {
        // This is a simple approximation. For accurate conversion, you'd need more complex methods.
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * (END_WAVELENGTH - START_WAVELENGTH) / (NUM_SAMPLES - 1);
            if (wavelength < 500) {
                result[i] = r * (500 - wavelength) / 100 + g * (wavelength - 400) / 100;
            } else if (wavelength < 600) {
                result[i] = g * (600 - wavelength) / 100 + b * (wavelength - 500) / 100;
            } else {
                result[i] = b;
            }
        }
        return result;
    }

    // Spectrum to RGB conversion
    std::array<float, 3> to_rgb() const {
        // This is a simple approximation. For accurate conversion, you'd need color matching functions.
        float r = 0, g = 0, b = 0;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * (END_WAVELENGTH - START_WAVELENGTH) / (NUM_SAMPLES - 1);
            if (wavelength < 500) {
                r += data_[i] * (500 - wavelength) / 100;
                g += data_[i] * (wavelength - 400) / 100;
            } else if (wavelength < 600) {
                g += data_[i] * (600 - wavelength) / 100;
                b += data_[i] * (wavelength - 500) / 100;
            } else {
                b += data_[i];
            }
        }
        float max_val = std::max({r, g, b});
        if (max_val > 0) {
            r /= max_val;
            g /= max_val;
            b /= max_val;
        }
        return {r, g, b};
    }

    // Generate spectrum for a given color temperature
    static spectrum blackbody(float temperature) {
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * (END_WAVELENGTH - START_WAVELENGTH) / (NUM_SAMPLES - 1);
            result[i] = blackbody_radiance(wavelength * 1e-9f, temperature);
        }
        return result.normalize();
    }

    // Generate D series illuminant spectrum
    static spectrum d_illuminant(float cct) {
        spectrum result;
        float x = 0, y = 0;

        if (cct <= 7000) {
            x = -4.6070e9f / (cct*cct*cct) + 2.9678e6f / (cct*cct) + 0.09911e3f / cct + 0.244063f;
        } else {
            x = -2.0064e9f / (cct*cct*cct) + 1.9018e6f / (cct*cct) + 0.24748e3f / cct + 0.237040f;
        }
        y = -3.000f * x * x + 2.870f * x - 0.275f;

        float M = 0.0241f + 0.2562f * x - 0.7341f * y;
        float M1 = (-1.3515f - 1.7703f * x + 5.9114f * y) / M;
        float M2 = (0.0300f - 31.4424f * x + 30.0717f * y) / M;

        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * (END_WAVELENGTH - START_WAVELENGTH) / (NUM_SAMPLES - 1);
            result[i] = s0(wavelength) + M1 * s1(wavelength) + M2 * s2(wavelength);
        }
        return result.normalize();
    }

    // Convenience functions for common illuminants
    static spectrum d50() { return d_illuminant(5000); }
    static spectrum d65() { return d_illuminant(6500); }

    spectrum& normalize() {
        float max_val = *std::max_element(data_.begin(), data_.end());
        if (max_val > 0) {
            for (float& val : data_) {
                val /= max_val;
            }
        }
        return *this;
    }

private:
    std::vector<float> data_;

    static float blackbody_radiance(float wavelength, float temperature) {
        const float h = 6.62607015e-34f; // Planck constant
        const float c = 299792458.0f;    // Speed of light
        const float k = 1.380649e-23f;   // Boltzmann constant
        float exp_term = std::exp((h * c) / (wavelength * k * temperature)) - 1.0f;
        return (2.0f * h * c * c) / (std::pow(wavelength, 5) * exp_term);
    }

    // S0, S1, and S2 functions for D series illuminants
    // These would be implemented based on CIE standard data
    static float s0(float wavelength) {
        static const std::vector<std::pair<float, float>> data = {
            {400, 0.04f}, {405, 3.02f}, {410, 6.00f}, {415, 17.80f}, {420, 29.60f},
            {425, 42.45f}, {430, 55.30f}, {435, 56.30f}, {440, 57.30f}, {445, 59.55f},
            {450, 61.80f}, {455, 61.65f}, {460, 61.50f}, {465, 65.15f}, {470, 68.80f},
            {475, 70.80f}, {480, 72.80f}, {485, 76.60f}, {490, 80.40f}, {495, 83.85f},
            {500, 87.30f}, {505, 88.50f}, {510, 89.70f}, {515, 92.20f}, {520, 94.70f},
            {525, 95.95f}, {530, 97.20f}, {535, 98.45f}, {540, 99.70f}, {545, 100.45f},
            {550, 101.20f}, {555, 101.60f}, {560, 102.00f}, {565, 101.70f}, {570, 101.40f},
            {575, 101.10f}, {580, 100.80f}, {585, 100.90f}, {590, 101.00f}, {595, 101.20f},
            {600, 101.40f}, {605, 101.00f}, {610, 100.60f}, {615, 100.00f}, {620, 99.40f},
            {625, 98.70f}, {630, 98.00f}, {635, 97.55f}, {640, 97.10f}, {645, 97.00f},
            {650, 96.90f}, {655, 96.75f}, {660, 96.60f}, {665, 96.25f}, {670, 95.90f},
            {675, 95.40f}, {680, 94.90f}, {685, 94.55f}, {690, 94.20f}, {695, 93.50f},
            {700, 92.80f}, {705, 92.65f}, {710, 92.50f}, {715, 92.20f}, {720, 91.90f},
            {725, 90.70f}, {730, 89.50f}, {735, 88.65f}, {740, 87.80f}, {745, 86.70f},
            {750, 85.60f}, {755, 85.05f}, {760, 84.50f}, {765, 83.50f}, {770, 82.50f},
            {775, 81.70f}, {780, 80.90f}, {785, 80.25f}, {790, 79.60f}, {795, 79.10f},
            {800, 78.60f}
        };
        return interpolate(data, wavelength);
    }

    static float s1(float wavelength) {
        static const std::vector<std::pair<float, float>> data = {
            {400, 0.02f}, {405, 2.26f}, {410, 4.50f}, {415, 13.45f}, {420, 22.40f},
            {425, 32.20f}, {430, 42.00f}, {435, 41.30f}, {440, 40.60f}, {445, 41.10f},
            {450, 41.60f}, {455, 39.80f}, {460, 38.00f}, {465, 40.20f}, {470, 42.40f},
            {475, 41.50f}, {480, 40.60f}, {485, 41.60f}, {490, 42.60f}, {495, 43.45f},
            {500, 44.30f}, {505, 44.15f}, {510, 44.00f}, {515, 45.30f}, {520, 46.60f},
            {525, 47.10f}, {530, 47.60f}, {535, 48.00f}, {540, 48.40f}, {545, 48.45f},
            {550, 48.50f}, {555, 48.70f}, {560, 48.90f}, {565, 48.55f}, {570, 48.20f},
            {575, 47.70f}, {580, 47.20f}, {585, 47.20f}, {590, 47.20f}, {595, 47.25f},
            {600, 47.30f}, {605, 47.00f}, {610, 46.70f}, {615, 46.25f}, {620, 45.80f},
            {625, 45.30f}, {630, 44.80f}, {635, 44.45f}, {640, 44.10f}, {645, 43.80f},
            {650, 43.50f}, {655, 43.40f}, {660, 43.30f}, {665, 43.10f}, {670, 42.90f},
            {675, 42.70f}, {680, 42.50f}, {685, 42.40f}, {690, 42.30f}, {695, 42.00f},
            {700, 41.70f}, {705, 41.65f}, {710, 41.60f}, {715, 41.50f}, {720, 41.40f},
            {725, 40.80f}, {730, 40.20f}, {735, 39.80f}, {740, 39.40f}, {745, 38.90f},
            {750, 38.40f}, {755, 38.15f}, {760, 37.90f}, {765, 37.40f}, {770, 36.90f},
            {775, 36.50f}, {780, 36.10f}, {785, 35.75f}, {790, 35.40f}, {795, 35.15f},
            {800, 34.90f}
        };
        return interpolate(data, wavelength);
    }
    static float s2(float wavelength) {
        static const std::vector<std::pair<float, float>> data = {
            {400, 0.00f}, {405, 1.00f}, {410, 2.00f}, {415, 3.00f}, {420, 4.00f},
            {425, 6.25f}, {430, 8.50f}, {435, 8.15f}, {440, 7.80f}, {445, 7.25f},
            {450, 6.70f}, {455, 6.00f}, {460, 5.30f}, {465, 5.70f}, {470, 6.10f},
            {475, 4.55f}, {480, 3.00f}, {485, 2.10f}, {490, 1.20f}, {495, 0.05f},
            {500, -1.10f}, {505, -0.80f}, {510, -0.50f}, {515, -0.60f}, {520, -0.70f},
            {525, -0.95f}, {530, -1.20f}, {535, -1.90f}, {540, -2.60f}, {545, -3.35f},
            {550, -4.10f}, {555, -4.65f}, {560, -5.20f}, {565, -5.55f}, {570, -5.90f},
            {575, -6.10f}, {580, -6.30f}, {585, -6.30f}, {590, -6.30f}, {595, -6.15f},
            {600, -6.00f}, {605, -5.80f}, {610, -5.60f}, {615, -5.40f}, {620, -5.20f},
            {625, -5.00f}, {630, -4.80f}, {635, -4.65f}, {640, -4.50f}, {645, -4.35f},
            {650, -4.20f}, {655, -4.15f}, {660, -4.10f}, {665, -4.05f}, {670, -4.00f},
            {675, -3.95f}, {680, -3.90f}, {685, -3.85f}, {690, -3.80f}, {695, -3.75f},
            {700, -3.70f}, {705, -3.65f}, {710, -3.60f}, {715, -3.55f}, {720, -3.50f},
            {725, -3.50f}, {730, -3.50f}, {735, -3.45f}, {740, -3.40f}, {745, -3.35f},
            {750, -3.30f}, {755, -3.25f}, {760, -3.20f}, {765, -3.15f}, {770, -3.10f},
            {775, -3.05f}, {780, -3.00f}, {785, -2.95f}, {790, -2.90f}, {795, -2.85f},
            {800, -2.80f}
        };
        return interpolate(data, wavelength);
    }

    // Interpolation function (should be implemented if not already present)
    static float interpolate(const std::vector<std::pair<float, float>>& data, float x) {
        auto it = std::lower_bound(data.begin(), data.end(), x,
                                   [](const std::pair<float, float>& p, float val) {
                                       return p.first < val;
                                   });

        if (it == data.begin()) return it->second;
        if (it == data.end()) return (it-1)->second;

        auto prev = it - 1;
        float t = (x - prev->first) / (it->first - prev->first);
        return prev->second + t * (it->second - prev->second);
    }
};