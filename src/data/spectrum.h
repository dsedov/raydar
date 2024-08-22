#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <stdexcept>


class Spectrum {
public:
    static constexpr int RESPONSE_SAMPLES = 361; // 1nm resolution from 380nm to 740nm
    static constexpr float START_WAVELENGTH = 380.0f;
    static constexpr float END_WAVELENGTH = 740.0f;

    Spectrum(int num_wavelengths) : data_(num_wavelengths) {}
    Spectrum(const float* data, int num_wavelengths) : data_(data, data + num_wavelengths) {}
    Spectrum(float r, float g, float b, int num_wavelengths) : data_(num_wavelengths) {
        float wavelength_step = (END_WAVELENGTH - START_WAVELENGTH) / (num_wavelengths - 1);

        for (int i = 0; i < num_wavelengths; ++i) {
            float wavelength = START_WAVELENGTH + i * wavelength_step;
            data_[i] = r * rgb_to_spectrum(wavelength, 0) +
                       g * rgb_to_spectrum(wavelength, 1) +
                       b * rgb_to_spectrum(wavelength, 2);
        }
    }
    float& operator[](int index) { return data_[index]; }
    const float& operator[](int index) const { return data_[index]; }
    
    int num_wavelengths() const { return static_cast<int>(data_.size()); }

    color to_rgb() const {
        float r = 0, g = 0, b = 0;
        float wavelength_step = (END_WAVELENGTH - START_WAVELENGTH) / (num_wavelengths() - 1);

        for (int i = 0; i < num_wavelengths(); ++i) {
            float wavelength = START_WAVELENGTH + i * wavelength_step;
            r += data_[i] * rgb_to_spectrum(wavelength, 0);
            g += data_[i] * rgb_to_spectrum(wavelength, 1);
            b += data_[i] * rgb_to_spectrum(wavelength, 2);
        }

        // Normalize
        float max_component = std::max({r, g, b, 1e-6f});
        return color(r / max_component, g / max_component, b / max_component);
    }

    // Other methods as needed (e.g., arithmetic operations, etc.)

private:
    std::vector<float> data_;
    // Approximate CIE 1931 color matching functions
    static float rgb_to_spectrum(float wavelength, int channel) {
        // Simplified RGB to spectrum conversion
        if (channel == 0) { // Red
            return (wavelength >= 580 && wavelength <= 645) ? 1.0f : 0.0f;
        } else if (channel == 1) { // Green
            return (wavelength >= 500 && wavelength <= 565) ? 1.0f : 0.0f;
        } else { // Blue
            return (wavelength >= 435 && wavelength <= 500) ? 1.0f : 0.0f;
        }
    }
    
};
