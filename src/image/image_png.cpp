#include "image_png.h"

ImagePNG::ImagePNG(int width, int height) : Image(width, height) {

    _channels = 3; // RGB
    _row_size = width * _channels;
    
    _image_buffer = std::vector<png_byte>(height * _row_size);
    for (int y = 0; y < height_; y++) {
        png_bytep row = _image_buffer.data() + y * _row_size;
        for (int x = 0; x < width_; x++) {
            png_bytep pixel = row + x * _channels;
            pixel[0] = 0;   // Red channel
            pixel[1] = 0;   // Green channel
            pixel[2] = 0;   // Blue channel
        }
    }

}
void ImagePNG::save(const char * filename) {

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
        row_pointers[y] = _image_buffer.data() + y * _row_size;
    }
    png_write_image(png, row_pointers.data());

    png_write_end(png, NULL);

    // Clean up
    png_destroy_write_struct(&png, &info);
    fclose(file);
}

ImagePNG::~ImagePNG() = default;