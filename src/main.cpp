#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"


#include "data/hittable_list.h"
#include "data/bvh.h"

#include "render.h"
#include "image/image_png.h"
#include "helpers/settings.h"

#include "usd/light.h"
#include "usd/material.h"
#include "usd/camera.h"
#include "usd/geo.h"
#include "usd/loader.h"

#include "helpers/strings.h"


int main(int argc, char *argv[]) {

    settings settings(argc, argv);
    if(settings.error > 0) return 1;

    // Test color spaces:
    color test(0.5, 0.5, 0.5);
    test.set_color_space(color::ColorSpace::SRGB);
    color srgb = test.to_srgb();
    std::cout << "sRGB: " << std::fixed << std::setprecision(2) << srgb.x() << "," << srgb.y() << "," << srgb.z() << std::endl;
    color rgb_lin = test.to_rgb();
    std::cout << "RGB_lin: " << std::fixed << std::setprecision(2) << rgb_lin.x() << "," << rgb_lin.y() << "," << rgb_lin.z() << std::endl;
    color xyz_test = test.to_xyz(whitepoint::D65());
    std::cout << "XYZ: " << std::fixed << std::setprecision(2) << xyz_test.x() << "," << xyz_test.y() << "," << xyz_test.z() << std::endl;
    color lab_test = test.to_lab(whitepoint::D65());
    std::cout << "Lab: " << std::fixed << std::setprecision(2) << lab_test.x() << "," << lab_test.y() << "," << lab_test.z() << std::endl;
    color hsl_test = test.to_hsl();
    std::cout << "HSL: " << std::fixed << std::setprecision(2) << hsl_test.x() << "," << hsl_test.y() << "," << hsl_test.z() << std::endl;



    return 0;



    // Validate RGB -> XYZ -> RGB
    color red(1.0, 0.0, 0.0);
    color red_xyz = red.to_xyz();
    color red_from_xyz_validation = red_xyz.to_rgb();
    std::cout << "Red: " << std::fixed << std::setprecision(2) << red.x() << "," << red.y() << "," << red.z() << " Red XYZ validation: " << red_from_xyz_validation.x() << "," << red_from_xyz_validation.y() << "," << red_from_xyz_validation.z() << std::endl;

    // Validate Spectrum -> XYZ -> RGB
    Spectrum red_spectrum(1.0, 0.0, 0.0);
    color red_xyz_fromSpectrum = red_spectrum.to_XYZ(Observer(Observer::CIE1931_2Deg, 31, 400, 700));
    color red_from_xyz_from_spectrum = red_xyz_fromSpectrum.to_rgb();
    std::cout << "XYZ: "  << std::fixed << std::setprecision(2) << red_xyz.x() << "," << red_xyz.y() << "," << red_xyz.z() << std::endl;
    std::cout << "XYZ from spectrum: " << std::fixed << std::setprecision(2) << red_xyz_fromSpectrum.x() << "," << red_xyz_fromSpectrum.y() << "," << red_xyz_fromSpectrum.z() << std::endl;
    std::cout << "Red: " << std::fixed << std::setprecision(2) << red.x() << "," << red.y() << "," << red.z() << " Red XYZ validation: " << red_from_xyz_validation.x() << "," << red_from_xyz_validation.y() << "," << red_from_xyz_validation.z() << std::endl;
    std::cout << "Red: " << std::fixed << std::setprecision(2) << red.x() << "," << red.y() << "," << red.z() << " Red from spectrum: " << red_from_xyz_from_spectrum.x() << "," << red_from_xyz_from_spectrum.y() << "," << red_from_xyz_from_spectrum.z() << std::endl;

    return 0;
    

    hittable_list world;
    hittable_list lights;

    // TESTING SPECTRAL CODE:
    // Macbeth ColorChecker colors
    std::vector<color> macbeth_colors = {
        color(0.4360, 0.4050, 0.3720), // Dark Skin
        color(0.7690, 0.5690, 0.4490), // Light Skin
        color(0.3440, 0.4740, 0.6510), // Blue Sky
        color(0.3050, 0.4790, 0.2930), // Foliage
        color(0.5540, 0.4240, 0.6240), // Blue Flower
        color(0.2110, 0.6760, 0.7660), // Bluish Green
        color(0.8460, 0.5430, 0.0160), // Orange
        color(0.3340, 0.3330, 0.5250), // Purplish Blue
        color(0.7980, 0.3370, 0.3400), // Moderate Red
        color(0.4440, 0.2940, 0.5380), // Purple
        color(0.5870, 0.6110, 0.3890), // Yellow Green
        color(0.8960, 0.7110, 0.0680), // Orange Yellow
        color(0.0350, 0.0680, 0.4690), // Blue
        color(0.2480, 0.5570, 0.3290), // Green
        color(0.6510, 0.2900, 0.2750), // Red
        color(0.9530, 0.9330, 0.0440), // Yellow
        color(0.7810, 0.2450, 0.5730), // Magenta
        color(0.0920, 0.7140, 0.7810), // Cyan
        color(0.9770, 0.9800, 0.9820), // White
        color(0.7480, 0.7500, 0.7480), // Neutral 8
        color(0.5360, 0.5380, 0.5360), // Neutral 6.5
        color(0.3620, 0.3630, 0.3610), // Neutral 5
        color(0.2070, 0.2080, 0.2070), // Neutral 3.5
        color(0.0910, 0.0910, 0.0910)  // Black
    };
    // Initialize SpectralConverter
    Observer observer(Observer::CIE1931_2Deg, Spectrum::RESPONSE_SAMPLES, Spectrum::START_WAVELENGTH, Spectrum::END_WAVELENGTH);


    for (const auto& c : macbeth_colors) {
        Spectrum s = Spectrum(c.x(), c.y(), c.z());
        color c2 = s.to_rgb(observer);
        std::cout << "In: " << c << " Out: " << c2 << std::endl;
    }

    // IMAGE
    ImagePNG image(settings.image_width, settings.image_height, Spectrum::RESPONSE_SAMPLES, observer);

    // LOAD USD FILE
    rd::usd::loader loader(settings.usd_file);

    // LOAD CAMERA
    rd::core::camera camera = rd::usd::camera::extractCameraProperties(loader.findFirstCamera());


    // LOAD MATERIALS
    std::cout << "Loading materials from USD stage" << std::endl;
    auto error_material = make_shared<rd::core::constant>(color(1.0, 0.0, 0.0));
    std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials = rd::usd::material::load_materials_from_stage(loader.get_stage());
    materials["error"] = error_material;

    // LOAD GEOMETRY
    std::cout << "Loading geometry from USD stage" << std::endl;
    std::vector<std::shared_ptr<rd::core::mesh>> scene_meshes = rd::usd::geo::extractMeshesFromUsdStage(loader.get_stage(), materials);
    
    // LOAD AREA LIGHTS
    std::cout << "Loading area lights from USD stage" << std::endl;
    std::vector<std::shared_ptr<rd::core::area_light>> area_lights = rd::usd::light::extractAreaLightsFromUsdStage(loader.get_stage());

    for(const auto& light : area_lights){
         world.add(light);
         lights.add(light);
    }
    

    render render(image, camera);
    render.samples_per_pixel = settings.samples;
    render.max_depth = settings.max_depth;


    std::cout << "Scene meshes size: " << scene_meshes.size() << std::endl;
    // for each mesh in sceneMeshes
    int bvh_meshes_size = 0;
    for (const auto& mesh : scene_meshes) {
        std::vector<rd::core::mesh> meshes;
        meshes = bvh_node::split(*mesh, meshes);
        
        for(const auto& m : meshes) {   
            world.add(make_shared<rd::core::mesh>(m));
            bvh_meshes_size++;
        }
    }
    std::cout << "BVH meshes size: " << bvh_meshes_size << std::endl;

    std::cout << "Building BVH" << std::endl;
    auto bvh_shared = make_shared<bvh_node>(world);
    world = hittable_list(bvh_shared);


    std::cout << "Rendering scene" << std::endl;
    int seconds_to_render = render.mtpool_bucket_prog_render(world, lights);



    // Save the image with the new file name

    image.normalize();
    image.save(settings.get_file_name(image.width(), image.height(), settings.samples, seconds_to_render).c_str());

    return 0;
}
