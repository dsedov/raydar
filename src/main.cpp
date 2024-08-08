#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"

#include "data/hittable.h"
#include "data/hittable_list.h"
#include "data/sphere.h"

#include "camera.h"
#include "image/image_png.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/sphere.h>


int main() {

    int image_width = 2048;
    int image_height = 1024;

    ImagePNG image(image_width, image_height);

    camera camera(image);
    camera.samples_per_pixel = 256;
    camera.max_depth = 10;

    hittable_list world;

    world.add(make_shared<sphere>(point3(0,0,-1), 0.5));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100));

    camera.mt_render(world);

    
    image.save("output.png");
    return 0;
}

