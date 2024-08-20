#pragma once
#include <iostream>
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
#include <random>
#include "../core/vec3.h" // Assuming this is where your color type is defined

class spectrum {
public:
    static constexpr int NUM_SAMPLES = 61; // 400nm to 700nm with 5nm steps
    static constexpr float START_WAVELENGTH = 400.0f;
    static constexpr float END_WAVELENGTH = 700.0f;
    static constexpr float WAVELENGTH_STEP = (END_WAVELENGTH - START_WAVELENGTH) / (NUM_SAMPLES - 1);

    spectrum() : data_(NUM_SAMPLES, 0.0f) {}
    explicit spectrum(float value) : data_(NUM_SAMPLES, value) {}
    spectrum(const float* values, int count) : data_(values, values + std::min(count, NUM_SAMPLES)) {}

    float& operator[](int index) { return data_[index]; }
    const float& operator[](int index) const { return data_[index]; }

    // Monte Carlo sampling method
    float sample_wavelength(float& pdf) const {
        float total = 0.0f;
        for (float value : data_) {
            total += value;
        }
        
        float r = random_float() * total;
        float cumsum = 0.0f;
        int index = 0;
        
        for (; index < NUM_SAMPLES; ++index) {
            cumsum += data_[index];
            if (cumsum > r) break;
        }
        
        float wavelength = START_WAVELENGTH + index * WAVELENGTH_STEP;
        pdf = data_[index] / total;
        return wavelength;
    }

    // RGB to Spectrum conversion (using a more accurate method)
    static spectrum from_rgb(float r, float g, float b) {
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * WAVELENGTH_STEP;
            result[i] = rgb_to_spectral(wavelength, r, g, b);
        }
        return result;
    }

    // Spectrum to RGB conversion (using color matching functions)
    color to_rgb() const {
        float r = 0, g = 0, b = 0;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            float wavelength = START_WAVELENGTH + i * WAVELENGTH_STEP;
            r += data_[i] * color_matching_function_x(wavelength);
            g += data_[i] * color_matching_function_y(wavelength);
            b += data_[i] * color_matching_function_z(wavelength);
        }
        float scale = 1.0f / NUM_SAMPLES;
        return color(r * scale, g * scale, b * scale);
    }
        float value_at_wavelength(float wavelength) const {
        int index = std::clamp(static_cast<int>((wavelength - START_WAVELENGTH) / WAVELENGTH_STEP), 0, NUM_SAMPLES - 1);
        return data_[index];
    }

    // Method to set the value at a specific wavelength
    void set_value_at_wavelength(float wavelength, float value) {
        int index = std::clamp(static_cast<int>((wavelength - START_WAVELENGTH) / WAVELENGTH_STEP), 0, NUM_SAMPLES - 1);
        data_[index] = value;
    }

    // Method to create a single-wavelength spectrum
    static spectrum single_wavelength(float wavelength, float intensity = 1.0f) {
        spectrum result;
        result.set_value_at_wavelength(wavelength, intensity);
        return result;
    }

    // Operator overloads for spectrum arithmetic
    spectrum operator+(const spectrum& other) const {
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.data_[i] = data_[i] + other.data_[i];
        }
        return result;
    }

    spectrum operator*(float scalar) const {
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.data_[i] = data_[i] * scalar;
        }
        return result;
    }

    spectrum operator*(const spectrum& other) const {
        spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.data_[i] = data_[i] * other.data_[i];
        }
        return result;
    }
    // Other methods (blackbody, d_illuminant, etc.) remain the same

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

    // Implement more accurate RGB to spectral conversion
    static float rgb_to_spectral(float wavelength, float r, float g, float b) {
        // This is a placeholder. Implement a more accurate conversion method.
        if (wavelength < 500) {
            return r * (500 - wavelength) / 100 + g * (wavelength - 400) / 100;
        } else if (wavelength < 600) {
            return g * (600 - wavelength) / 100 + b * (wavelength - 500) / 100;
        } else {
            return b;
        }
    }

    // Implement color matching functions (CIE 1931 standard colorimetric observer)
    static float color_matching_function_x(float wavelength) {
        // Implement the x̄(λ) function
    }

    static float color_matching_function_y(float wavelength) {
        // Implement the ȳ(λ) function
    }

    static float color_matching_function_z(float wavelength) {
        // Implement the z̄(λ) function
    }

    // Utility function for random number generation
    static float random_float() {
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_real_distribution<> dis(0.0, 1.0);
        return dis(gen);
    }
};

class spectral_image {
public:
    spectral_image(int width, int height) 
        : width_(width), height_(height) {
        image_buffer_ = std::vector<spectrum>(width * height);
        sample_counts_ = std::vector<int>(width * height, 0);
    }

    virtual ~spectral_image() {}

    virtual void save(const char* filename) = 0;

    void add_sample(int x, int y, const spectrum& sample) {
        int pixel_index = y * width_ + x;
        image_buffer_[pixel_index] = image_buffer_[pixel_index] + sample;
        sample_counts_[pixel_index]++;
    }

    spectrum get_pixel(int x, int y) const {
        int pixel_index = y * width_ + x;
        int count = sample_counts_[pixel_index];
        return count > 0 ? image_buffer_[pixel_index] * (1.0f / count) : spectrum();
    }

    int get_sample_count(int x, int y) const {
        return sample_counts_[y * width_ + x];
    }

    void clear() {
        std::fill(image_buffer_.begin(), image_buffer_.end(), spectrum());
        std::fill(sample_counts_.begin(), sample_counts_.end(), 0);
    }

    int width() const { return width_; }
    int height() const { return height_; }

private:
    int width_;
    int height_;
    std::vector<spectrum> image_buffer_;
    std::vector<int> sample_counts_;
};