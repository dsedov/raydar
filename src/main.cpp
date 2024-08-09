#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"

#include "data/hittable.h"
#include "data/hittable_list.h"
#include "data/sphere.h"


#include "camera.h"
#include "material.h"
#include "image/image_png.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/sphere.h>


int main() {

    int image_width = 2048;
    int image_height = 1024;

    ImagePNG image(image_width, image_height);

    camera camera(image);
    camera.samples_per_pixel = 128;
    camera.max_depth = 10;

    hittable_list world;

    auto material_metal  = make_shared<metal>(color(0.8, 0.8, 0.8),0.2);
    auto material_glass = make_shared<dielectric>(1.5);
    auto material_glass_inside = make_shared<dielectric>(1.0/1.5);
    auto material_ground = make_shared<lambertian>(color(0.0, 0.5, 0.5));
    auto material_ground2 = make_shared<lambertian>(color(0.5, 0.5, 0.5));

    world.add(make_shared<sphere>(point3(0, 0,-1.5), 0.5,material_glass));
    world.add(make_shared<sphere>(point3(0, 0,-1.5), 0.45,material_glass_inside));
    world.add(make_shared<sphere>(point3(1, 0,-1.5), 0.5,material_ground));
    world.add(make_shared<sphere>(point3(-1,0,-1.5), 0.5,material_ground));
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100,material_ground2));

    camera.mt_render(world);

    
    image.save("output.png");
    return 0;
}

