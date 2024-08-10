#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"

#include "data/hittable.h"
#include "data/hittable_list.h"
#include "data/sphere.h"
#include "data/obj.h"


#include "camera.h"
#include "material.h"
#include "image/image_png.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/sphere.h>

#include "helpers/settings.h"

int main(int argc, char *argv[]) {

    settings settings(argc, argv);
    if(settings.error > 0) return 1;
    

    ImagePNG image(settings.image_width, settings.image_height);

    camera camera(image);
    camera.vfov     = 50;
    camera.lookfrom = point3(-4,4,1);
    camera.lookat   = point3(0,0,-1);
    camera.vup      = vec3(0,1,0);
    camera.samples_per_pixel = settings.samples;
    camera.max_depth = settings.max_depth;

    hittable_list world;

    auto material_metal  = make_shared<metal>(color(0.8, 0.8, 0.8),0.2);
    auto material_glass = make_shared<dielectric>(1.1, 0.00);
    auto material_glass_inside = make_shared<dielectric>(1.0/1.5);
    auto material_ground = make_shared<lambertian>(color(0.0, 0.5, 0.5));
    auto material_ground2 = make_shared<lambertian>(color(0.5, 0.5, 0.5));

    //world.add(make_shared<obj>("tet.obj", material_glass));
    
    world.add(make_shared<sphere>(point3(0, 0,-1.5), 0.5,material_glass));
    world.add(make_shared<sphere>(point3(0, 0,-1.5), 0.45,material_ground2));

    world.add(make_shared<sphere>(point3(1, 0,-1.5), 0.5,material_glass));
    world.add(make_shared<sphere>(point3(1, 0,-1.5), 0.45,material_ground2));

    world.add(make_shared<sphere>(point3(-1,0,-1.5), 0.5,material_glass));
    world.add(make_shared<sphere>(point3(-1,0,-1.5), 0.45,material_ground2));
    
    world.add(make_shared<sphere>(point3(0,-100.5,-1), 100,material_ground2));
    
    camera.mt_render(world);
    image.save(settings.image_file.c_str());
    return 0;
}
