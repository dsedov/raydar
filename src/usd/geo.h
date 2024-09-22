#ifndef RD_USD_GEO_H
#define RD_USD_GEO_H

#include <cstdio>
#include <vector>

#include "../data/hittable_list.h"
#include "../data/bvh.h"
#include "../core/mesh.h"
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
    extern rd::core::mesh* loadFromUsdMesh(const pxr::UsdGeomMesh& usd_mesh, rd::core::material* mat, const pxr::GfMatrix4d& transform) ;
    extern void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<rd::core::mesh*>& meshes, std::unordered_map<std::string, rd::core::material*>& materials);
    extern std::vector<rd::core::mesh*> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::unordered_map<std::string, rd::core::material*>& materials);
    

}
#endif