#pragma once
#include "image.h"
#include <png.h>
#include <vector>

class ImagePNG : public Image {
public:
    ImagePNG(int width, int height, int num_wavelengths, const observer& observer) 
        : Image(width, height, num_wavelengths, observer) {}

    ~ImagePNG() override = default;

    void save(const char* filename) override {
        std::vector<png_byte> png_buffer(height_ * width_ * 3);
        static const interval intensity(0.000, 0.999);

        for (int y = 0; y < height_; y++) {
            png_bytep row = png_buffer.data() + y * width_ * 3;
            for (int x = 0; x < width_; x++) {
                png_bytep pixel = row + x * 3;
                spectrum spectrum = get_pixel(x, y);
                color rgb = spectrum.to_rgb(observer_);

                pixel[0] = int(255.999 * intensity.clamp(linear_to_gamma(rgb.x()))); // Red channel
                pixel[1] = int(255.999 * intensity.clamp(linear_to_gamma(rgb.y()))); // Green channel
                pixel[2] = int(255.999 * intensity.clamp(linear_to_gamma(rgb.z()))); // Blue channel
            }
        }

        // Create the PNG file
        FILE* file = fopen(filename, "wb");
        if (!file) {
            printf("Failed to create the PNG file\n");
            return;
        }

        // Initialize the PNG structures
        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            printf("Failed to initialize the PNG write struct\n");
            fclose(file);
            return;
        }
        png_infop info = png_create_info_struct(png);
        if (!info) {
            printf("Failed to initialize the PNG info struct\n");
            png_destroy_write_struct(&png, NULL);
            fclose(file);
            return;
        }

        // Set up error handling
        if (setjmp(png_jmpbuf(png))) {
            printf("Failed to set up PNG error handling\n");
            png_destroy_write_struct(&png, &info);
            fclose(file);
            return;
        }

        // Set the PNG file as the output
        png_init_io(png, file);

        // Set the image properties
        png_set_IHDR(png, info, width_, height_, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                     PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

        // Write the image data
        png_write_info(png, info);
        std::vector<png_bytep> row_pointers(height_);
        for (int y = 0; y < height_; y++) {
            row_pointers[y] = png_buffer.data() + y * width_ * 3;
        }
        png_write_image(png, row_pointers.data());
        png_write_end(png, NULL);

        // Clean up
        png_destroy_write_struct(&png, &info);
        fclose(file);
    }
};
