#include <cstdio>
#include <png.h>
#include <vector>

#include "image/image_png.h"

int main() {

    ImagePNG image(2048, 1024);
    image.save("output.png");
    return 0;
}