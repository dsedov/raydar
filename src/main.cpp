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
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
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
    props.lookUp = up;

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
// Helper function to get the local transform of a prim
pxr::GfMatrix4d getLocalTransform(const pxr::UsdPrim& prim) {
    if (prim.IsA<pxr::UsdGeomXformable>()) {
        pxr::UsdGeomXformable xformable(prim);
        bool resetsXformStack;
        std::vector<pxr::UsdGeomXformOp> xformOps = xformable.GetOrderedXformOps(&resetsXformStack);
        
        pxr::GfMatrix4d localTransform(1); // Identity matrix
        for (const auto& op : xformOps) {
            pxr::GfMatrix4d mat;
            op.GetOpTransform(pxr::UsdTimeCode::Default());
            localTransform = localTransform * mat;
        }
        return localTransform;
    }
    return pxr::GfMatrix4d(1); // Identity matrix for non-transformable prims
}

// Recursive function to traverse the USD hierarchy and extract meshes
void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, 
                                   const pxr::GfMatrix4d& parentTransform, 
                                   std::vector<std::shared_ptr<mesh>>& meshes,
                                   std::shared_ptr<material> mat) {


    pxr::GfMatrix4d localTransform = getLocalTransform(prim);
    pxr::GfMatrix4d fullTransform = parentTransform * localTransform;

    if (prim.IsA<pxr::UsdGeomMesh>()) {
        pxr::UsdGeomMesh usdMesh(prim);
        
        // Create a new mesh object with the accumulated transform
        auto newMesh = std::make_shared<mesh>(usdMesh, mat, fullTransform);
        meshes.push_back(newMesh);
    }

    // Recurse through children
    for (const auto& child : prim.GetChildren()) {
        traverseStageAndExtractMeshes(child, fullTransform, meshes, mat);
    }
}
std::vector<std::shared_ptr<mesh>> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::shared_ptr<material> mat) {
    std::vector<std::shared_ptr<mesh>> meshes;
    pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
    pxr::GfMatrix4d rootTransform(1); // Start with identity matrix

    traverseStageAndExtractMeshes(rootPrim, rootTransform, meshes, mat);

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
    std::vector<std::shared_ptr<mesh>> sceneMeshes = extractMeshesFromUsdStage(stage, material_lambert);
    

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
    // for each mesh in sceneMeshes
    for (const auto& mesh : sceneMeshes) {
        world.add(mesh);
    }
    
    
    camera.mt_render(world);
    image.save(settings.image_file.c_str());
    return 0;
}
