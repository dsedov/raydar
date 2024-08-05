#include <cstdio>
#include <png.h>
#include <vector>

#include "image/image_png.h"

int main() {

    ImagePNG image(2048, 1024);
    
    for(int x = 100; x < 200; x++) {
        for(int y = 100; y < 200; y++) {
            image.set_pixel(x, y, 1.0, 0.0, 0.0);
        }
    }

    image.save("output.png");
    return 0;
}