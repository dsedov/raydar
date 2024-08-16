#ifndef RD_USD_LOADER_H
#define RD_USD_LOADER_H

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

namespace rd::usd {
    class loader {
        public:
            loader(std::string file_path) {
                std::cout << "Loading USD file: " << file_path << std::endl;
                stage = pxr::UsdStage::Open(file_path);
                // Load metersPerUnit from the stage
                double metersPerUnit = UsdGeomGetStageMetersPerUnit(stage);
                std::cout << "Meters per unit: " << metersPerUnit << std::endl;
            }
            pxr::UsdStageRefPtr get_stage() {
                return stage;
            }
            pxr::UsdGeomCamera findFirstCamera() {
                // Traverse all prims in the stage
                for (const auto& prim : stage->TraverseAll()) {
                    // Check if the prim is a camera
                    if (prim.IsA<pxr::UsdGeomCamera>()) {
                        // Return the first camera found
                        return pxr::UsdGeomCamera(prim);
                    }
                }
                // If no camera is found, return an invalid camera
                std::cout << "No camera found in the stage" << std::endl;
                return pxr::UsdGeomCamera();
            }
            ~loader() {}
        private:
            pxr::UsdStageRefPtr stage;
    };
}
#endif