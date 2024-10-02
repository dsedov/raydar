#pragma once
#include "image.h"
#include <png.h>
#include <vector>
#include <cstdio>

class ImageSPD : public Image {
public:
    ImageSPD(int width, int height, int num_wavelengths, observer * observer) 
        : Image(width, height, num_wavelengths, observer) {}

    ~ImageSPD() override = default;
    
    static ImageSPD load(const char* filename, observer * obs) {
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

        ImageSPD image(width, height, spectrum::RESPONSE_SAMPLES, obs);

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
    void save_spectrum(const char* filename, float gamma, float exposure) override {
        gamma_ = gamma;
        exposure_ = exposure;
        // Get file pointer for writing
        FILE* file = fopen(filename, "wb");
        if (!file) {
            printf("Failed to create the SPD file\n");
            return;
        }

        // Write resolution in binary format
        fwrite(&width_, sizeof(int), 1, file);
        fwrite(&height_, sizeof(int), 1, file);
        fwrite(&gamma_, sizeof(float), 1, file);
        fwrite(&exposure_, sizeof(float), 1, file);
        fwrite(&spectrum::RESPONSE_SAMPLES, sizeof(int), 1, file);

        // Write the SPD data to the file
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                spectrum spectrum = get_pixel(x, y);
                auto data = spectrum.get_data();
                fwrite(data.data(), sizeof(float), data.size(), file);
            }
        }

        fclose(file);
    }
    void load_spectrum(const char* filename) override {
        // Load the SPD data from the file
        FILE* file = fopen(filename, "rb");
        if (!file) {
            printf("Failed to open the SPD file for reading\n");
            return;
        }

        fread(&width_, sizeof(int), 1, file);
        fread(&height_, sizeof(int), 1, file);
        fread(&gamma_, sizeof(float), 1, file);
        fread(&exposure_, sizeof(float), 1, file);
        fread(&num_wavelengths_, sizeof(int), 1, file);

        // Make the image buffer the correct size
        row_size_ = width_ * num_wavelengths_;
        image_buffer_ = std::vector<float>(width_ * height_ * num_wavelengths_ );
        std::fill(image_buffer_.begin(), image_buffer_.end(), 0.0f);

        // Read the SPD data from the file
        for (int y = 0; y < height_; y++) {
            for (int x = 0; x < width_; x++) {
                std::vector<float> pixel_data(num_wavelengths_);
                fread(pixel_data.data(), sizeof(float), num_wavelengths_, file);
                spectrum spectrum(pixel_data);
                set_pixel(x, y, spectrum);
            }
        }

        fclose(file);
    }
    float exposure_ = 1.0f;
    float gamma_ = 2.2f;
    

    void save(const char* filename, float gamma = 2.2, float exposure = 1.0) override {
        std::cout << "Saving image to " << filename << " with gamma:" << gamma << " and exposure:" << exposure << std::endl;
        std::vector<png_byte> png_buffer(height_ * width_ * 3);
        static const interval intensity(0.000, 0.999);

        for (int y = 0; y < height_; y++) {
            png_bytep row = png_buffer.data() + y * width_ * 3;
            for (int x = 0; x < width_; x++) {
                png_bytep pixel = row + x * 3;
                spectrum spectrum = get_pixel(x, y) * std::pow(2.0, exposure);
                color rgb = spectrum.to_rgb(observer_);

                pixel[0] = int(255.999 * intensity.clamp(linear_to_gamma2(rgb.x(), gamma))); // Red channel
                pixel[1] = int(255.999 * intensity.clamp(linear_to_gamma2(rgb.y(), gamma))); // Green channel
                pixel[2] = int(255.999 * intensity.clamp(linear_to_gamma2(rgb.z(), gamma))); // Blue channel
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
