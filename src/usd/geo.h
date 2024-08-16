



#ifndef RD_USD_GEO_H
#define RD_USD_GEO_H

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

namespace rd::usd::geo {
    // Recursive function to traverse the USD hierarchy and extract meshes
    void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<std::shared_ptr<usd_mesh>>& meshes, std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials) {
        
        if (prim.IsA<pxr::UsdGeomMesh>()) {
            std::cout << std::endl << "Prim Path: " << prim.GetPath().GetString() << std::endl;
            pxr::UsdGeomMesh usdMesh(prim);
            pxr::GfMatrix4d xform = pxr::UsdGeomXformable(usdMesh).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
            // Extract material name
            std::shared_ptr<rd::core::material> mat = materials["error"];
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
    std::vector<std::shared_ptr<usd_mesh>> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials) {
        std::vector<std::shared_ptr<usd_mesh>> meshes;
        pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
        traverseStageAndExtractMeshes(rootPrim, meshes, materials);
        return meshes;
    }

}
#endif