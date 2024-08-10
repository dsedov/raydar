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

#include "helpers/cxxopts.hpp"

int main(int argc, char *argv[]) {

    // DEFAULTS
    std::string file = "";
    int image_width = 1024;
    int image_height = 768;
    int samples = 4;
  


    cxxopts::Options options("Raydar", "Spectral USD Renderer");
    // Define options for file, resolution, and samples
    options.add_options()
        ("f,file", "File name", cxxopts::value<std::string>())
        ("r,resolution", "Resolution (two integers)", cxxopts::value<std::vector<int>>()->default_value("1024,768"))
        ("s,samples", "Number of samples", cxxopts::value<int>()->default_value("4"))
        ("h,help", "Print usage");

    auto result = options.parse(argc, argv);    
    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        return 0;
    }

    // FILE
    if (result.count("file")) {
        file = result["file"].as<std::string>();
        std::cout << "File: " << file << std::endl;
    } else {
        std::cerr << "Error: File name must be provided." << std::endl;
        return 1;
    }

    // RESOLUTION
    if (result.count("resolution")) {
        std::vector<int> resolution = result["resolution"].as<std::vector<int>>();
        if (resolution.size() != 2) {
            std::cerr << "Error: Resolution must consist of exactly two integers." << std::endl;
            return 1;
        }
        std::cout << "Resolution: " << resolution[0] << "x" << resolution[1] << std::endl;
        image_width = resolution[0];
        image_height = resolution[1];
    } 
    std::cout << "Resolution: " << image_width << "x" << image_height << std::endl;
    

    // SAMPLES
    if (result.count("samples")) samples = result["samples"].as<int>();

    std::cout << "Samples: " << samples << std::endl;
    

    ImagePNG image(image_width, image_height);

    camera camera(image);
    camera.vfov     = 50;
    camera.lookfrom = point3(-4,4,1);
    camera.lookat   = point3(0,0,-1);
    camera.vup      = vec3(0,1,0);
    camera.samples_per_pixel = samples;
    camera.max_depth = 10;

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

    
    image.save("output.png");
    return 0;
}

