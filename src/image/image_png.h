#pragma once
#include "image.h"
#include <png.h>
#include <vector>

class ImagePNG : public Image {
public:
    ImagePNG(int width, int height);
    ~ImagePNG() override;
    void save(const char * filename) override;
private:
    std::vector<png_byte> _image_buffer;

};
