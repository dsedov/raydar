
#ifndef RD_USD_LIGHT_H
#define RD_USD_LIGHT_H
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
#include "../core/light.h"
namespace rd::usd::light {
    struct AreaLight {
        pxr::GfVec3f color;
        float diffuse;
        float exposure;
        float height;
        float width;
        float intensity;
        float spread = 1.0;
        bool normalize;
        pxr::GfVec3f shadowColor;
        bool shadowEnable;
        float specular;
        std::string textureFilePath;
        pxr::VtArray<pxr::GfVec3d> vertices;
        pxr::GfVec3d rotation;
        pxr::GfVec3d scale;
        pxr::GfVec3d translation;
        pxr::VtArray<float> spectrumValues;
        point3 Q;
        vec3 u;
        vec3 v;
    };
    extern AreaLight extractAreaLightProperties(const pxr::UsdPrim& prim, const pxr::GfMatrix4d& transform, int verbose = 0);
    extern std::vector<rd::core::area_light*> extractAreaLightsFromUsdStage(const pxr::UsdStageRefPtr& stage, observer * observer) ;
}

#endif