

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
#include <pxr/usd/usdGeom/metrics.h>

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
        pxr::UsdStagePtr stage = camera.GetPrim().GetStage();

        // Get stage meters per unit for conversion
        float metersPerUnit = UsdGeomGetStageMetersPerUnit(stage);

        // Extract horizontal and vertical aperture
        float horizontalAperture, verticalAperture;
        camera.GetHorizontalApertureAttr().Get(&horizontalAperture);
        camera.GetVerticalApertureAttr().Get(&verticalAperture);

        // Convert aperture to meters
        horizontalAperture *= metersPerUnit * 100;
        verticalAperture *= metersPerUnit * 100;


        // Calculate FOV
        float focalLength;
        if (camera.GetFocalLengthAttr().Get(&focalLength)) {
            focalLength *= metersPerUnit * 100;
            props.fov = 2 * std::atan((horizontalAperture * 0.5) / focalLength);
            props.fov = props.fov * (180.0 / M_PI);
        }

        // Get the local-to-world transform using ComputeLocalToWorldTransform
        pxr::GfMatrix4d localToWorld = camera.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

        // Extract translate (camera position)
        props.center = localToWorld.ExtractTranslation();

        // Calculate lookAt
        pxr::GfVec3d forward(0, 0, -1);
        forward = forward * localToWorld.ExtractRotationMatrix();
        props.lookAt = props.center + forward;

        // Calculate lookUp
        pxr::GfVec3d up(0, 1, 0);
        props.lookUp = up * localToWorld.ExtractRotationMatrix();

        // Extract rotateXYZ (if needed)
        pxr::GfVec3d rotXYZ = localToWorld.ExtractRotation().Decompose(pxr::GfVec3d::XAxis(), pxr::GfVec3d::YAxis(), pxr::GfVec3d::ZAxis());
        props.rotateXYZ = pxr::GfVec3f(rotXYZ[0], rotXYZ[1], rotXYZ[2]);

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