#ifndef RD_USD_MATERIAL_H
#define RD_USD_MATERIAL_H

#include <cstdio>
#include <vector>
#include "../core/material.h"

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../data/bvh.h"

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/vec3d.h>
#include <pxr/base/gf/rotation.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/vt/array.h>

#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdLux/rectLight.h>


namespace rd::usd::material {

    extern pxr::UsdShadeShader findShaderInNodeGraph(const pxr::UsdPrim& nodeGraphPrim);
    extern void findMaterialsRecursive(const pxr::UsdPrim& prim, std::vector<pxr::UsdShadeMaterial>& materials);
    extern pxr::UsdShadeShader GetShader(const pxr::UsdShadeMaterial& usdMaterial);
    extern std::unordered_map<std::string, rd::core::material*> load_materials_from_stage(const pxr::UsdStageRefPtr& stage) ;

}
#endif