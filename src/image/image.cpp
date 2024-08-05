#include "image.h"

Image::Image(int width, int height, int channels) {
    width_ = width;
    height_ = height;
    channels_ = channels;
    row_size_ = width_ * channels_;
    image_buffer_ = std::vector<float>(width * height * channels);
    std::fill(image_buffer_.begin(), image_buffer_.end(), 0.0);
}

void Image::set_pixel(int x, int y, float r, float g, float b) {
        int index = y * row_size_ + x * channels_;
        image_buffer_[index] = r;
        image_buffer_[index + 1] = g;
        image_buffer_[index + 2] = b;
}