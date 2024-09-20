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


    std::vector<std::vector<std::vector<vec3>>> lookup_table;
    std::ifstream file("lookup_table.bin", std::ios::binary);
    if (file.good()) {
        std::cout << "Loading existing lookup table..." << std::endl;
        lookup_table = spectrum::load_lookup_tables(0.01);
        spectrum::setLookupTable(lookup_table, 0.01);
        color target_rgb = color(0.83, 0.25, 0.40);
        target_rgb.set_color_space(color::ColorSpace::SRGB);
        std::cout << "Target RGB: " << target_rgb.x() << "," << target_rgb.y() << "," << target_rgb.z() << std::endl;
        spectrum target_spectrum = spectrum(target_rgb);
        std::cout << "Target spectrum: "  << std::endl;
        color possible_rgb = target_spectrum.to_rgb(observer(observer::CIE1931_2Deg, 31, 400, 700)).to_srgb();
        std::cout << "Target RGB: " << target_rgb.x() << "," << target_rgb.y() << "," << target_rgb.z() << std::endl;
        std::cout << "Possible RGB: " << possible_rgb.x() << "," << possible_rgb.y() << "," << possible_rgb.z() << std::endl;
    } else {
        std::cout << "Lookup table not found. Computing new lookup table..." << std::endl;
        spectrum::compute_lookup_tables(0.01);
        return 0;
    }


    

    hittable_list world;
    hittable_list lights;

    // Initialize SpectralConverter
    observer observer(observer::CIE1931_2Deg, spectrum::RESPONSE_SAMPLES, spectrum::START_WAVELENGTH, spectrum::END_WAVELENGTH);



    // IMAGE
    ImagePNG image(settings.image_width, settings.image_height, spectrum::RESPONSE_SAMPLES, observer);

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
