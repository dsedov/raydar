#pragma once
#include "image.h"
#include <png.h>
#include <vector>

class ImagePNG : public Image {
public:
    ImagePNG(int width, int height, int num_wavelengths, const observer& observer) 
        : Image(width, height, num_wavelengths, observer) {}

    ~ImagePNG() override = default;
    
    static ImagePNG load(const char* filename, const observer& obs) {
        FILE* file = fopen(filename, "rb");
        if (!file) {
            throw std::runtime_error("Failed to open the PNG file for reading");
        }

        png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) {
            fclose(file);
            throw std::runtime_error("Failed to create PNG read struct");
        }

        png_infop info = png_create_info_struct(png);
        if (!info) {
            png_destroy_read_struct(&png, NULL, NULL);
            fclose(file);
            throw std::runtime_error("Failed to create PNG info struct");
        }

        if (setjmp(png_jmpbuf(png))) {
            png_destroy_read_struct(&png, &info, NULL);
            fclose(file);
            throw std::runtime_error("Error during PNG file reading");
        }

        png_init_io(png, file);
        png_read_info(png, info);

        int width = png_get_image_width(png, info);
        int height = png_get_image_height(png, info);
        png_byte color_type = png_get_color_type(png, info);
        png_byte bit_depth = png_get_bit_depth(png, info);

        if (bit_depth == 16)
            png_set_strip_16(png);

        if (color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_palette_to_rgb(png);

        if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
            png_set_expand_gray_1_2_4_to_8(png);

        if (png_get_valid(png, info, PNG_INFO_tRNS))
            png_set_tRNS_to_alpha(png);

        if (color_type == PNG_COLOR_TYPE_RGB ||
            color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_PALETTE)
            png_set_filler(png, 0xFF, PNG_FILLER_AFTER);

        if (color_type == PNG_COLOR_TYPE_GRAY ||
            color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
            png_set_gray_to_rgb(png);

        png_read_update_info(png, info);

        std::vector<png_bytep> row_pointers(height);
        for (int y = 0; y < height; y++) {
            row_pointers[y] = new png_byte[png_get_rowbytes(png, info)];
        }

        png_read_image(png, row_pointers.data());

        ImagePNG image(width, height, spectrum::RESPONSE_SAMPLES, obs);

        // Convert pixel by pixel to spectrum
        for (int y = 0; y < height; y++) {
            png_bytep row = row_pointers[y];
            for (int x = 0; x < width; x++) {
                png_bytep px = &(row[x * 4]);
                color rgb(px[0] / 255.0, px[1] / 255.0, px[2] / 255.0);
                spectrum s = spectrum(rgb);
                image.set_pixel(x, y, s);
            }
        }

        for (int y = 0; y < height; y++) {
            delete[] row_pointers[y];
        }

        png_destroy_read_struct(&png, &info, NULL);
        fclose(file);

        return image;
    }
    spectrum uv_value(double u, double v) const {
        return get_pixel(u * width_, v * height_);
    }   
    void save(const char* filename) override {
        std::vector<png_byte> png_buffer(height_ * width_ * 3);
        static const interval intensity(0.000, 0.999);

        for (int y = 0; y < height_; y++) {
            png_bytep row = png_buffer.data() + y * width_ * 3;
            for (int x = 0; x < width_; x++) {
                png_bytep pixel = row + x * 3;
                spectrum spectrum = get_pixel(x, y);
                color rgb = spectrum.to_rgb(observer_);
                
                std::cout << rgb << std::endl;

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
