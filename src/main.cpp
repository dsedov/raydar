#include <cstdio>
#include <png.h>
#include <vector>
#include "raydar.h"

#include "data/hittable.h"
#include "data/hittable_list.h"
#include "data/sphere.h"
#include "data/usd_mesh.h"
#include "data/bvh.h"


#include "camera.h"
#include "material.h"
#include "image/image_png.h"
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdLux/rectLight.h>
#include "helpers/settings.h"

struct AreaLight {
    pxr::GfVec3f color;
    float diffuse;
    float exposure;
    float height;
    float width;
    float intensity;
    bool normalize;
    pxr::GfVec3f shadowColor;
    bool shadowEnable;
    float specular;
    std::vector<pxr::GfVec3f> vertices;
    pxr::GfVec3f rotation;
    pxr::GfVec3f scale;
    pxr::GfVec3d translation;
};
struct MaterialProperties {
    std::string fullPath;
    float metallic = 0.0f;
    float roughness = 0.5f;
    pxr::GfVec3f diffuseColor = pxr::GfVec3f(0.8f, 0.8f, 0.8f);
    pxr::GfVec3f emissiveColor = pxr::GfVec3f(0.0f, 0.0f, 0.0f);
    float ior = 1.5f;
};
struct CameraProperties {
    pxr::GfVec3f rotateXYZ;
    pxr::GfVec3d translate;
    pxr::GfVec3d center;
    pxr::GfVec3d lookAt;
    pxr::GfVec3d lookUp;
    float fov;
};
CameraProperties extractCameraProperties(const pxr::UsdGeomCamera& camera) {
    CameraProperties props;

    // Extract rotateXYZ
    pxr::UsdAttribute rotateAttr = camera.GetPrim().GetAttribute(pxr::TfToken("xformOp:rotateXYZ"));
    if (rotateAttr) {
        rotateAttr.Get(&props.rotateXYZ);
    }

    // Extract translate
    pxr::UsdAttribute translateAttr = camera.GetPrim().GetAttribute(pxr::TfToken("xformOp:translate"));
    if (translateAttr) {
        translateAttr.Get(&props.translate);
    }
    // Extract up
    pxr::UsdAttribute upAttr = camera.GetPrim().GetAttribute(pxr::TfToken("primvars:arnold:up"));
    if (upAttr) {
        pxr::VtArray<pxr::GfVec3f> upArray;
        upAttr.Get(&upArray);
        if (!upArray.empty()) {
            props.lookUp = pxr::GfVec3d(upArray[0][0], upArray[0][1], upArray[0][2]);
        }
    }

    // Extract FOV
    pxr::UsdAttribute fovAttr = camera.GetPrim().GetAttribute(pxr::TfToken("primvars:arnold:fov"));
    if (fovAttr) {
        pxr::VtArray<float> fovArray;
        fovAttr.Get(&fovArray);
        if (!fovArray.empty()) {
            props.fov = fovArray[0];
        }
    }

    // Calculate center (camera position)
    props.center = props.translate;

    // Create rotations for each axis
    pxr::GfRotation rotX(pxr::GfVec3d(1, 0, 0), props.rotateXYZ[0]);
    pxr::GfRotation rotY(pxr::GfVec3d(0, 1, 0), props.rotateXYZ[1]);
    pxr::GfRotation rotZ(pxr::GfVec3d(0, 0, 1), props.rotateXYZ[2]);

    // Combine rotations (order: Z, Y, X)
    pxr::GfRotation finalRotation = rotZ * rotY * rotX;

    // Create transformation matrix using rotation and translation
    pxr::GfMatrix4d rotationMatrix;
    rotationMatrix.SetRotate(finalRotation);
    pxr::GfMatrix4d translationMatrix(1);
    translationMatrix.SetTranslate(props.translate);
    pxr::GfMatrix4d transformMatrix = rotationMatrix * translationMatrix;


    pxr::GfVec3d forward(-transformMatrix.GetColumn(2)[0],
                         -transformMatrix.GetColumn(2)[1],
                         transformMatrix.GetColumn(2)[2]);

    // Set lookAt and lookUp
    props.lookAt = props.center - forward;


    return props;
}
pxr::UsdGeomCamera findFirstCamera(const pxr::UsdStageRefPtr& stage) {
    // Traverse all prims in the stage
    for (const auto& prim : stage->TraverseAll()) {
        // Check if the prim is a camera
        if (prim.IsA<pxr::UsdGeomCamera>()) {
            // Return the first camera found
            return pxr::UsdGeomCamera(prim);
        }
    }
    // If no camera is found, return an invalid camera
    return pxr::UsdGeomCamera();
}

// Recursive function to traverse the USD hierarchy and extract meshes
void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<std::shared_ptr<usd_mesh>>& meshes, std::unordered_map<std::string, std::shared_ptr<material>> materials) {
    
    if (prim.IsA<pxr::UsdGeomMesh>()) {
        std::cout << std::endl << "Prim Path: " << prim.GetPath().GetString() << std::endl;
        pxr::UsdGeomMesh usdMesh(prim);
        pxr::GfMatrix4d xform = pxr::UsdGeomXformable(usdMesh).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
        // Extract material name
        std::shared_ptr<material> mat = materials["error"];
        std::string materialName = "No Material";
        pxr::UsdShadeMaterial boundMaterial = pxr::UsdShadeMaterialBindingAPI(usdMesh).ComputeBoundMaterial();
        if (boundMaterial) {
            materialName = boundMaterial.GetPrim().GetPath().GetString();
            mat = materials[materialName];
        }
        
        std::cout << "Material Name: " << materialName << std::endl;

        auto newMesh = std::make_shared<usd_mesh>(usdMesh, mat, xform);
        meshes.push_back(newMesh);
    }
    for (const auto& child : prim.GetChildren()) traverseStageAndExtractMeshes(child, meshes, materials);
}
std::vector<std::shared_ptr<usd_mesh>> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::unordered_map<std::string, std::shared_ptr<material>> materials) {
    std::vector<std::shared_ptr<usd_mesh>> meshes;
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    traverseStageAndExtractMeshes(rootPrim, meshes, materials);
    return meshes;
}


std::shared_ptr<material> createMaterialFromProperties(const MaterialProperties& props) {
    // Implement this function based on your material system
    // For now, we'll return a lambertian material as a placeholder
    return std::make_shared<advanced_pbr_material>(
        0.8, // base_weight (guessed)
        color(props.diffuseColor[0], props.diffuseColor[1], props.diffuseColor[2]), // base_color
        props.metallic, // base_metalness
        0.2, // specular_weight (guessed)
        color(1.0, 1.0, 1.0), // specular_color (guessed)
        props.roughness, // specular_roughness
        props.ior, // specular_ior
        0.0, // transmission_weight (guessed)
        color(1.0, 1.0, 1.0), // transmission_color (guessed)
        1.0, // emission_luminance (guessed)
        color(props.emissiveColor[0], props.emissiveColor[1], props.emissiveColor[2]) // emission_color
    );
     return std::make_shared<lambertian>(color(props.diffuseColor[0], props.diffuseColor[1], props.diffuseColor[2]));

    
}

pxr::UsdShadeShader findShaderInNodeGraph(const pxr::UsdPrim& nodeGraphPrim) {
    for (const auto& child : nodeGraphPrim.GetChildren()) {
        if (child.IsA<pxr::UsdShadeShader>()) {
            return pxr::UsdShadeShader(child);
        }
    }
    return pxr::UsdShadeShader();
}

MaterialProperties extractMaterialProperties(const pxr::UsdShadeMaterial& usdMaterial) {
    MaterialProperties props;
    props.fullPath = usdMaterial.GetPath().GetString();

    // Look for MaterialX and UsdPreviewSurface NodeGraphs
    pxr::UsdPrim materialXNodeGraph = usdMaterial.GetPrim().GetChild(pxr::TfToken("MaterialX"));
    pxr::UsdPrim previewSurfaceNodeGraph = usdMaterial.GetPrim().GetChild(pxr::TfToken("UsdPreviewSurface"));

    pxr::UsdShadeShader materialXShader, previewSurfaceShader;

    if (materialXNodeGraph) {
        materialXShader = findShaderInNodeGraph(materialXNodeGraph);
    }

    if (previewSurfaceNodeGraph) {
        previewSurfaceShader = findShaderInNodeGraph(previewSurfaceNodeGraph);
    }

    // Prefer MaterialX if available, otherwise use UsdPreviewSurface
    pxr::UsdShadeShader shaderToUse = materialXShader ? materialXShader : previewSurfaceShader;

    if (shaderToUse) {
        pxr::VtValue value;

        if (shaderToUse.GetInput(pxr::TfToken("metallic")).Get(&value)) {
            props.metallic = value.Get<float>();
        }
        if (shaderToUse.GetInput(pxr::TfToken("roughness")).Get(&value)) {
            props.roughness = value.Get<float>();
        }
        if (shaderToUse.GetInput(pxr::TfToken("diffuseColor")).Get(&value)) {
            props.diffuseColor = value.Get<pxr::GfVec3f>();
        }
        if (shaderToUse.GetInput(pxr::TfToken("emissiveColor")).Get(&value)) {
            props.emissiveColor = value.Get<pxr::GfVec3f>();
        }
        if (shaderToUse.GetInput(pxr::TfToken("ior")).Get(&value)) {
            props.ior = value.Get<float>();
        }
    }

    return props;
}

void findMaterialsRecursive(const pxr::UsdPrim& prim, std::vector<pxr::UsdShadeMaterial>& materials) {
    if (prim.IsA<pxr::UsdShadeMaterial>()) {
        materials.push_back(pxr::UsdShadeMaterial(prim));
    }
    
    for (const auto& child : prim.GetChildren()) {
        findMaterialsRecursive(child, materials);
    }
}

std::unordered_map<std::string, std::shared_ptr<material>> loadMaterialsFromStage(const pxr::UsdStageRefPtr& stage) {
    std::unordered_map<std::string, std::shared_ptr<material>> materials;

    std::vector<pxr::UsdShadeMaterial> usdMaterials;
    findMaterialsRecursive(stage->GetPseudoRoot(), usdMaterials);

    for (const auto& usdMaterial : usdMaterials) {
        MaterialProperties props = extractMaterialProperties(usdMaterial);
        std::shared_ptr<material> mat = createMaterialFromProperties(props);
        materials[props.fullPath] = mat;

        std::cout << "Loaded material: " << props.fullPath << std::endl;
        std::cout << "  Metallic: " << props.metallic << std::endl;
        std::cout << "  Roughness: " << props.roughness << std::endl;
        std::cout << "  Diffuse Color: " << props.diffuseColor << std::endl;
        std::cout << "  Emissive Color: " << props.emissiveColor << std::endl;
        std::cout << "  IOR: " << props.ior << std::endl;
    }

    return materials;
}

std::vector<AreaLight> extractAreaLightsFromUsdStage(const pxr::UsdStageRefPtr& stage) {
    std::vector<AreaLight> areaLights;
    
    for (const auto& prim : stage->TraverseAll()) {
        if (prim.IsA<pxr::UsdLuxRectLight>()) {
            std::cout << "RectLight: " << prim.GetPath().GetString() << std::endl;
            AreaLight light = AreaLight();
            areaLights.push_back(light);
        }
    }
    
    return areaLights;
}
int main(int argc, char *argv[]) {

    settings settings(argc, argv);
    if(settings.error > 0) return 1;
    
    // IMAGE
    ImagePNG image(settings.image_width, settings.image_height);

    // LOAD USD FILE
    pxr::UsdStageRefPtr stage = pxr::UsdStage::Open(settings.usd_file);

    // LOAD CAMERA
    CameraProperties cameraProps;
    pxr::UsdGeomCamera firstCamera = findFirstCamera(stage);
    if (firstCamera) cameraProps = extractCameraProperties(firstCamera);
    else {
        std::cout << "No camera found in the USD stage." << std::endl;
        return 1;
    }
    std::cout << "Camera Properties:" << std::endl;
    std::cout << "Look From: " << cameraProps.center << std::endl;
    std::cout << "Look At: " << cameraProps.lookAt << std::endl;
    std::cout << "Look Up: " << cameraProps.lookUp << std::endl;
    std::cout << "FOV: " << cameraProps.fov << std::endl<< std::endl;

    // LOAD MATERIALS
    auto error_material = make_shared<lambertian>(color(1.0, 0.0, 0.0));
    std::unordered_map<std::string, std::shared_ptr<material>> materials = loadMaterialsFromStage(stage);
    materials["error"] = error_material;

    // LOAD GEOMETRY
    std::vector<std::shared_ptr<usd_mesh>> sceneMeshes = extractMeshesFromUsdStage(stage, materials);
    
    // LOAD AREA LIGHTS
    std::vector<AreaLight> areaLights = extractAreaLightsFromUsdStage(stage);

    camera camera(image);
    camera.fov      = cameraProps.fov;
    camera.lookfrom = point3(cameraProps.center[0], cameraProps.center[1], cameraProps.center[2]);
    camera.lookat   = point3(cameraProps.lookAt[0], cameraProps.lookAt[1], cameraProps.lookAt[2]);
    camera.vup      = point3(cameraProps.lookUp[0], cameraProps.lookUp[1], cameraProps.lookUp[2]);
    camera.samples_per_pixel = settings.samples;
    camera.max_depth = settings.max_depth;



    hittable_list world;

    auto material_metal  = make_shared<metal>(color(0.8, 0.8, 0.8),0.2);
    auto material_glass = make_shared<dielectric>(1.1, 0.00);
    auto material_glass_inside = make_shared<dielectric>(1.0/1.5);
    auto material_ground = make_shared<lambertian>(color(0.0, 0.5, 0.5));


    

    std::cout << "Scene meshes size: " << sceneMeshes.size() << std::endl;
    // for each mesh in sceneMeshes
    int bvh_meshes_size = 0;
    for (const auto& mesh : sceneMeshes) {
        std::vector<usd_mesh> meshes;
        meshes = mesh->split(meshes);
        
        for(const auto& m : meshes) {   
            world.add(make_shared<usd_mesh>(m));
            bvh_meshes_size++;
        }
    }
    std::cout << "BVH meshes size: " << bvh_meshes_size << std::endl;

    world = hittable_list(make_shared<bvh_node>(world));

    camera.mt_render(world);
    image.save(settings.image_file.c_str());
    return 0;
}
