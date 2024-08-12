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


#include "helpers/settings.h"

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
void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<std::shared_ptr<usd_mesh>>& meshes, std::shared_ptr<material> mat) {
    std::cout << std::endl << "Prim Path: " << prim.GetPath().GetString() << std::endl;
    if (prim.IsA<pxr::UsdGeomMesh>()) {
        pxr::UsdGeomMesh usdMesh(prim);
        pxr::GfMatrix4d xform = pxr::UsdGeomXformable(usdMesh).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
        auto newMesh = std::make_shared<usd_mesh>(usdMesh, mat, xform);
        meshes.push_back(newMesh);
    }
    for (const auto& child : prim.GetChildren()) traverseStageAndExtractMeshes(child, meshes, mat);
}
std::vector<std::shared_ptr<usd_mesh>> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::shared_ptr<material> mat) {
    std::vector<std::shared_ptr<usd_mesh>> meshes;
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    traverseStageAndExtractMeshes(rootPrim, meshes, mat);
    return meshes;
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

    // TODO:LOAD GEOMETRY
    auto material_lambert= make_shared<lambertian>(color(0.5, 0.5, 0.5));
    std::vector<std::shared_ptr<usd_mesh>> sceneMeshes = extractMeshesFromUsdStage(stage, material_lambert);
    

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
    auto material_ground2 = make_shared<lambertian>(color(0.5, 0.5, 0.5));

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
