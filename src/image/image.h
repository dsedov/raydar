#pragma once
#include <iostream>
#include <vector>

class Image {
public:
    Image(int width, int height, int channels);
    virtual ~Image() {}
    virtual void save(const char * filename) = 0;

    void set_pixel(int x, int y, float r, float g, float b);
protected:
    int width_;
    int height_;
    int channels_;
    int row_size_;
    std::vector<float> image_buffer_;
};
