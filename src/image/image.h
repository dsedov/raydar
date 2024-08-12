#pragma once
#include <iostream>
#include <vector>
#include "data/color.h"

class Image {
public:
    Image(int width, int height, int channels) {
        width_ = width;
        height_ = height;
        channels_ = channels;
        row_size_ = width_ * channels_;
        image_buffer_ = std::vector<float>(width * height * channels);
        std::fill(image_buffer_.begin(), image_buffer_.end(), 0.0);
    }
    virtual ~Image() {}
    virtual void save(const char * filename) = 0;

    void set_pixel(int x, int y, float r, float g, float b) {
        int index = y * row_size_ + x * channels_;
        image_buffer_[index] = r;
        image_buffer_[index + 1] = g;
        image_buffer_[index + 2] = b;
    }
    void set_pixel(int x, int y, color color) {
        set_pixel(x, y, color.x(), color.y(), color.z());
    }
    color get_pixel(int x, int y) {
        int index = y * row_size_ + x * channels_;
        return color(image_buffer_[index], image_buffer_[index + 1], image_buffer_[index + 2]);
    }

    int width() { return width_;}
    int height() { return height_;}
protected:
    int width_;
    int height_;
    int channels_;
    int row_size_;
    std::vector<float> image_buffer_;
};
