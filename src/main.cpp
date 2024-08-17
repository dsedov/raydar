#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"


#include "data/hittable_list.h"
#include "data/quad.h"
#include "data/bvh.h"

#include "render.h"
#include "image/image_png.h"
#include "helpers/settings.h"

#include "usd/light.h"
#include "usd/material.h"
#include "usd/camera.h"
#include "usd/geo.h"
#include "usd/loader.h"

int main(int argc, char *argv[]) {

    settings settings(argc, argv);
    if(settings.error > 0) return 1;

    hittable_list world;

    // IMAGE
    ImagePNG image(settings.image_width, settings.image_height);

    // LOAD USD FILE
    rd::usd::loader loader(settings.usd_file);

    // LOAD CAMERA
    rd::core::camera camera = rd::usd::camera::extractCameraProperties(loader.findFirstCamera());


    // LOAD MATERIALS
    std::cout << "Loading materials from USD stage" << std::endl;
    auto error_material = make_shared<rd::core::constant>(color(1.0, 0.0, 0.0));
    auto light_material = make_shared<rd::core::light>(color(1.0, 1.0, 1.0), 1.0);
    std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials = rd::usd::material::loadMaterialsFromStage(loader.get_stage());
    materials["error"] = error_material;

    // LOAD GEOMETRY
    std::cout << "Loading geometry from USD stage" << std::endl;
    std::vector<std::shared_ptr<rd::core::mesh>> sceneMeshes = rd::usd::geo::extractMeshesFromUsdStage(loader.get_stage(), materials);
    
    // LOAD AREA LIGHTS
    std::cout << "Loading area lights from USD stage" << std::endl;
    std::vector<rd::usd::light::AreaLight> areaLights = rd::usd::light::extractAreaLightsFromUsdStage(loader.get_stage());
    for(const auto& light : areaLights) {
        shared_ptr<quad> light_quad = make_shared<quad>(light.Q, light.u, light.v, light_material);

        world.add(light_quad);
    }

    render render(image, camera);
    render.samples_per_pixel = settings.samples;
    render.max_depth = settings.max_depth;


    std::cout << "Scene meshes size: " << sceneMeshes.size() << std::endl;
    // for each mesh in sceneMeshes
    int bvh_meshes_size = 0;
    for (const auto& mesh : sceneMeshes) {
        std::vector<rd::core::mesh> meshes;
        meshes = bvh_node::split(*mesh, meshes);
        
        for(const auto& m : meshes) {   
            world.add(make_shared<rd::core::mesh>(m));
            bvh_meshes_size++;
        }
    }
    std::cout << "BVH meshes size: " << bvh_meshes_size << std::endl;

    world = hittable_list(make_shared<bvh_node>(world));

    int seconds_to_render = render.mtpool_prog_render(world);

    // Extract the file name and extension
    std::string file_name = settings.image_file;
    size_t dot_pos = file_name.find_last_of(".");
    std::string name = file_name.substr(0, dot_pos);
    std::string extension = file_name.substr(dot_pos);

    // Create the new file name with resolution and samples
    std::stringstream new_file_name;
    // Generate a simple 6-character alphanumeric UUID
    std::string uuid;
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::srand(std::time(nullptr));
    for (int i = 0; i < 6; ++i) {
        uuid += chars[rand() % chars.length()];
    }
    new_file_name << name << "_" << image.width() << "x" << image.height() << "_" << settings.samples << "spp" << "_" << seconds_to_render << "s" << "_" << uuid << extension;

    // Save the image with the new file name
    image.save(new_file_name.str().c_str());

    return 0;
}
