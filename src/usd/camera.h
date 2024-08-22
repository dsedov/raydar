

#ifndef RD_USD_CAMERA_H
#define RD_USD_CAMERA_H

#include <cstdio>
#include <vector>
#include "../raydar.h"

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../data/bvh.h"

#include "../core/camera.h"
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

    rd::core::camera extractCameraProperties(const pxr::UsdGeomCamera& usd_camera) {
        rd::core::camera camera;
        pxr::UsdStagePtr stage = usd_camera.GetPrim().GetStage();

        // Get stage meters per unit for conversion
        float metersPerUnit = UsdGeomGetStageMetersPerUnit(stage);

        // Extract horizontal and vertical aperture
        float horizontalAperture, verticalAperture;
        usd_camera.GetHorizontalApertureAttr().Get(&horizontalAperture);
        usd_camera.GetVerticalApertureAttr().Get(&verticalAperture);

        // Convert aperture to meters
        horizontalAperture *= metersPerUnit * 100;
        verticalAperture *= metersPerUnit * 100;


        // Calculate FOV
        float focalLength;
        if (usd_camera.GetFocalLengthAttr().Get(&focalLength)) {
            focalLength *= metersPerUnit * 100;
            camera.fov = 2 * std::atan((horizontalAperture * 0.5) / focalLength);
            camera.fov = camera.fov * (180.0 / M_PI);
        }

        // Get the local-to-world transform using ComputeLocalToWorldTransform
        pxr::GfMatrix4d localToWorld = usd_camera.ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());

        // Extract translate (camera position)
        camera.center = localToWorld.ExtractTranslation();

        // Calculate lookAt
        pxr::GfVec3d forward(0, 0, -1);
        forward = forward * localToWorld.ExtractRotationMatrix();
        camera.look_at = vec3(camera.center[0] + forward[0], camera.center[1] + forward[1], camera.center[2] + forward[2]);

        // Calculate lookUp
        pxr::GfVec3d up(0, 1, 0);
        camera.look_up = up * localToWorld.ExtractRotationMatrix();


        std::cout << "\nCamera Properties:" << std::endl;
        std::cout << "Look From: " << camera.center << std::endl;
        std::cout << "Look At: " << camera.look_at << std::endl;
        std::cout << "Look Up: " << camera.look_up << std::endl;
        std::cout << "FOV: " << camera.fov << std::endl<< std::endl;
        return camera;
    }
    
}
#endif