#pragma once
#include <iostream>
#include <vector>
#include "data/color.h"
#include "data/spectrum.h"

class Image {
public:
    Image(int width, int height, int num_wavelengths, std::vector<std::vector<std::vector<vec3>>> lookup_table, const Observer& observer) 
        : width_(width), height_(height), num_wavelengths_(num_wavelengths), lookup_table_(lookup_table), observer_(observer) {
        row_size_ = width_ * num_wavelengths_;
        image_buffer_ = std::vector<float>(width * height * num_wavelengths);
        std::fill(image_buffer_.begin(), image_buffer_.end(), 0.0f);
    }

    virtual ~Image() {}
    virtual void save(const char* filename) = 0;

    void set_pixel(int x, int y, const Spectrum& spectrum) {
        int index = y * row_size_ + x * num_wavelengths_;
        for (int i = 0; i < num_wavelengths_; ++i) {
            image_buffer_[index + i] = spectrum[i];
        }
    }
    void add_to_pixel(int x, int y, const Spectrum& spectrum) {
        int index = y * row_size_ + x * num_wavelengths_;
        for (int i = 0; i < num_wavelengths_; ++i) {
            image_buffer_[index + i] += spectrum[i];
        }
    }
    Spectrum get_pixel(int x, int y) const {
        int index = y * row_size_ + x * num_wavelengths_;
        return Spectrum(image_buffer_.data() + index);
    }

    // RGB 
    // RGB methods
    void set_pixel(int x, int y, float r, float g, float b) {
        set_pixel(x, y, Spectrum(color(r, g, b), lookup_table_));
    }

    void add_to_pixel(int x, int y, float r, float g, float b) {
        add_to_pixel(x, y, Spectrum(color(r, g, b), lookup_table_));
    }

    void add_to_pixel(int x, int y, const color& color) {
        add_to_pixel(x, y, color.x(), color.y(), color.z());
    }

    void set_pixel(int x, int y, const color& color) {
        set_pixel(x, y, color.x(), color.y(), color.z());
    }

    color get_pixel_rgb(int x, int y) const {
        return get_pixel(x, y).to_rgb(observer_);
    }

    void normalize() {
        float max_value = 0.0f;

        // Find the maximum value in the image
        for (const auto& value : image_buffer_) {
            max_value = std::max(max_value, value);
        }

        // Avoid division by zero
        if (max_value > 0.0f) {
            // Normalize all values
            for (auto& value : image_buffer_) {
                value /= max_value;
            }
        }
    }

    int width() const { return width_; }
    int height() const { return height_; }
    int num_wavelengths() const { return num_wavelengths_; }

protected:
    int width_;
    int height_;
    int num_wavelengths_;
    int row_size_;
    std::vector<float> image_buffer_;
    const Observer& observer_;
    std::vector<std::vector<std::vector<vec3>>> lookup_table_;
};
