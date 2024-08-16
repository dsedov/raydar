

#ifndef RD_USD_CAMERA_H
#define RD_USD_CAMERA_H

#include <cstdio>
#include <vector>
#include "../raydar.h"

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../data/quad.h"
#include "../data/usd_mesh.h"
#include "../data/bvh.h"

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

namespace rd::usd::camera {
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
        } else {
            pxr::UsdAttribute focalLengthAttr = camera.GetPrim().GetAttribute(pxr::TfToken("focalLength"));
            if (focalLengthAttr) {
                float focalLength;
                focalLengthAttr.Get(&focalLength);
                focalLength *= 100;
                std::cout << "Focal Length: " << focalLength << std::endl;
                props.fov = 2 * std::atan(36 / (2 *focalLength));
                props.fov = props.fov * (180.0 / M_PI);
                std::cout << "FOV: " << props.fov << std::endl;
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
}
#endif