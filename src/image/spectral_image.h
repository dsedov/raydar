#pragma once
#include <iostream>
#include <vector>
#include "../data/spectrum.h" // Assume this header defines the Spectrum class

class spectral_image {
public:
    spectral_image(int width, int height, int num_wavelengths) 
        : width_(width), height_(height), num_wavelengths_(num_wavelengths) {
        image_buffer_ = std::vector<std::vector<float>>(width * height, std::vector<float>(num_wavelengths, 0.0f));
        sample_counts_ = std::vector<std::vector<unsigned int>>(width * height, std::vector<unsigned int>(num_wavelengths, 0));
    }

    virtual ~spectral_image() {}

    virtual void save(const char* filename) = 0;

    void add_sample(int x, int y, float wavelength, float value) {
        int pixel_index = y * width_ + x;
        int wave_index = wavelength_to_index(wavelength);
        
        image_buffer_[pixel_index][wave_index] += value;
        sample_counts_[pixel_index][wave_index]++;
    }

    spectrum get_pixel(int x, int y) const {
        int pixel_index = y * width_ + x;
        std::vector<float> result(num_wavelengths_);
        
        for (int i = 0; i < num_wavelengths_; ++i) {
            unsigned int count = sample_counts_[pixel_index][i];
            result[i] = (count > 0) ? image_buffer_[pixel_index][i] / count : 0.0f;
        }
        
        return spectrum(result.data(), num_wavelengths_);
    }

    unsigned int get_total_sample_count(int x, int y) const {
        int pixel_index = y * width_ + x;
        unsigned int total = 0;
        for (int i = 0; i < num_wavelengths_; ++i) {
            total += sample_counts_[pixel_index][i];
        }
        return total;
    }

    void clear() {
        for (auto& pixel : image_buffer_) {
            std::fill(pixel.begin(), pixel.end(), 0.0f);
        }
        for (auto& pixel_counts : sample_counts_) {
            std::fill(pixel_counts.begin(), pixel_counts.end(), 0);
        }
    }

    int width() const { return width_; }
    int height() const { return height_; }
    int num_wavelengths() const { return num_wavelengths_; }

private:
    int wavelength_to_index(float wavelength) const {
        // Implement this based on your wavelength discretization
        // For example, if you're using 400-700nm with 10nm steps:
        return static_cast<int>((wavelength - 400) / 10);
    }

    int width_;
    int height_;
    int num_wavelengths_;
    std::vector<std::vector<float>> image_buffer_;
    std::vector<std::vector<unsigned int>> sample_counts_;
};