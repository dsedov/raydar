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
        // Improved RGB to spectrum conversion
        float wavelength_step = (END_WAVELENGTH - START_WAVELENGTH) / (num_wavelengths - 1);

        // Define base functions
        auto f1 = [](float w) { return std::max(0.0f, 1.0f - std::abs((w - 575.0f) / 100.0f)); };
        auto f2 = [](float w) { return std::max(0.0f, 1.0f - std::abs((w - 500.0f) / 100.0f)); };
        auto f3 = [](float w) { return std::max(0.0f, 1.0f - std::abs((w - 425.0f) / 100.0f)); };

        // Calculate weights
        float w1 = 1.0f * r - 0.5f * g - 0.5f * b;
        float w2 = -0.5f * r + 1.0f * g - 0.5f * b;
        float w3 = -0.5f * r - 0.5f * g + 1.0f * b;

        // Generate spectrum
        for (int i = 0; i < num_wavelengths; ++i) {
            float wavelength = START_WAVELENGTH + i * wavelength_step;
            data_[i] = w1 * f1(wavelength) + w2 * f2(wavelength) + w3 * f3(wavelength);
        }

        // Ensure non-negativity
        for (float& value : data_) {
            value = std::max(0.0f, value);
        }

        // Normalize the spectrum
        float max_value = *std::max_element(data_.begin(), data_.end());
        if (max_value > 0) {
            for (float& value : data_) {
                value /= max_value;
            }
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
            float wavelength_nm = wavelength / 1000.0f; // Convert to micrometers for the formula

            // Approximate CIE 1931 color matching functions
            float x = color_match_x(wavelength_nm);
            float y = color_match_y(wavelength_nm);
            float z = color_match_z(wavelength_nm);

            // Convert XYZ to RGB
            r += data_[i] * ( 3.2404542f * x - 1.5371385f * y - 0.4985314f * z);
            g += data_[i] * (-0.9692660f * x + 1.8760108f * y + 0.0415560f * z);
            b += data_[i] * ( 0.0556434f * x - 0.2040259f * y + 1.0572252f * z);
        }

        // Ensure non-negative values
        r = std::max(0.0f, r);
        g = std::max(0.0f, g);
        b = std::max(0.0f, b);

        // Apply inverse gamma correction, avoiding potential NaN
        r = (r > 0.0f) ? std::pow(r, 1.0f / 2.2f) : 0.0f;
        g = (g > 0.0f) ? std::pow(g, 1.0f / 2.2f) : 0.0f;
        b = (b > 0.0f) ? std::pow(b, 1.0f / 2.2f) : 0.0f;

        // Normalize and return, avoiding division by zero
        float max_component = std::max({r, g, b, 1e-6f});
        return color(r / max_component, g / max_component, b / max_component);
    }

    // Other methods as needed (e.g., arithmetic operations, etc.)

private:
    std::vector<float> data_;
    // Approximate CIE 1931 color matching functions
    static float color_match_x(float wavelength) {
        float t1 = (wavelength - 0.4420f) / (0.0800f);
        float t2 = (wavelength - 0.5998f) / (0.1700f);
        return 1.056f * std::exp(-0.5f * t1 * t1) - 0.062f * std::exp(-0.5f * t2 * t2);
    }

    static float color_match_y(float wavelength) {
        float t1 = (wavelength - 0.5688f) / (0.1000f);
        return 0.821f * std::exp(-0.5f * t1 * t1);
    }

    static float color_match_z(float wavelength) {
        float t1 = (wavelength - 0.4370f) / (0.0610f);
        float t2 = (wavelength - 0.4592f) / (0.0700f);
        return 1.217f * std::exp(-0.5f * t1 * t1) - 0.681f * std::exp(-0.5f * t2 * t2);
    }
    
};
