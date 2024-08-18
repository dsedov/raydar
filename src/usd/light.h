
#ifndef RD_USD_LIGHT_H
#define RD_USD_LIGHT_H

#include <cstdio>
#include <vector>
#include "../raydar.h"

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../core/light.h"
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

namespace rd::usd::light {
    struct AreaLight {
        pxr::GfVec3f color;
        float diffuse;
        float exposure;
        float height;
        float width;
        float intensity;
        bool normalize;
        pxr::GfVec3f shadowColor;
        bool shadowEnable;
        float specular;
        pxr::VtArray<pxr::GfVec3f> vertices;
        pxr::GfVec3f rotation;
        pxr::GfVec3f scale;
        pxr::GfVec3d translation;

        point3 Q;
        vec3 u;
        vec3 v;
    };
    AreaLight extractAreaLightProperties(const pxr::UsdPrim& prim, const pxr::GfMatrix4d& transform) {
        AreaLight light;
        
        // Extract properties
        pxr::UsdAttribute colorAttr = prim.GetAttribute(pxr::TfToken("inputs:color"));
        if (colorAttr) colorAttr.Get(&light.color);
        
        pxr::UsdAttribute diffuseAttr = prim.GetAttribute(pxr::TfToken("inputs:diffuse"));
        if (diffuseAttr) diffuseAttr.Get(&light.diffuse);
        
        pxr::UsdAttribute exposureAttr = prim.GetAttribute(pxr::TfToken("inputs:exposure"));
        if (exposureAttr) exposureAttr.Get(&light.exposure);
        
        pxr::UsdAttribute heightAttr = prim.GetAttribute(pxr::TfToken("inputs:height"));
        if (heightAttr) heightAttr.Get(&light.height);
        
        pxr::UsdAttribute widthAttr = prim.GetAttribute(pxr::TfToken("inputs:width"));
        if (widthAttr) widthAttr.Get(&light.width);
        
        pxr::UsdAttribute intensityAttr = prim.GetAttribute(pxr::TfToken("inputs:intensity"));
        if (intensityAttr) intensityAttr.Get(&light.intensity);
        
        pxr::UsdAttribute normalizeAttr = prim.GetAttribute(pxr::TfToken("inputs:normalize"));
        if (normalizeAttr) normalizeAttr.Get(&light.normalize);
        
        pxr::UsdAttribute shadowColorAttr = prim.GetAttribute(pxr::TfToken("inputs:shadow:color"));
        if (shadowColorAttr) shadowColorAttr.Get(&light.shadowColor);
        
        pxr::UsdAttribute shadowEnableAttr = prim.GetAttribute(pxr::TfToken("inputs:shadow:enable"));
        if (shadowEnableAttr) shadowEnableAttr.Get(&light.shadowEnable);
        
        pxr::UsdAttribute specularAttr = prim.GetAttribute(pxr::TfToken("inputs:specular"));
        if (specularAttr) specularAttr.Get(&light.specular);
        
        pxr::UsdAttribute verticesAttr = prim.GetAttribute(pxr::TfToken("primvars:arnold:vertices"));
        if (verticesAttr) {verticesAttr.Get(&light.vertices);
            for(auto& v : light.vertices) {
                std::cout << "Vertex: " << v << std::endl;
                v = transform.Transform(v);
                std::cout << "Vertex: " << v << std::endl;
            }
        }
        else {
            light.vertices = {{1, -1, 0}, {-1, -1, 0}, {-1, 1, 0}, {1, 1, 0}};
            for(auto& v : light.vertices) {
                std::cout << "Vertex: " << v << std::endl;
                v = transform.Transform(v);
                std::cout << "Vertex: " << v << std::endl;
            }
        }

        light.Q = point3(light.vertices[0][0], light.vertices[0][1], light.vertices[0][2]);
        light.v = vec3(light.vertices[1][0] - light.vertices[0][0],
                    light.vertices[1][1] - light.vertices[0][1],
                    light.vertices[1][2] - light.vertices[0][2]);
        light.u = vec3(light.vertices[3][0] - light.vertices[0][0],
                    light.vertices[3][1] - light.vertices[0][1],
                    light.vertices[3][2] - light.vertices[0][2]);
        
        pxr::UsdAttribute rotateAttr = prim.GetAttribute(pxr::TfToken("xformOp:rotateXYZ"));
        if (rotateAttr) rotateAttr.Get(&light.rotation);
        
        pxr::UsdAttribute scaleAttr = prim.GetAttribute(pxr::TfToken("xformOp:scale"));
        if (scaleAttr) scaleAttr.Get(&light.scale);
        
        pxr::UsdAttribute translateAttr = prim.GetAttribute(pxr::TfToken("xformOp:translate"));
        if (translateAttr) translateAttr.Get(&light.translation);
        
        return light;
    }
    std::vector<std::shared_ptr<rd::core::area_light>> extractAreaLightsFromUsdStage(const pxr::UsdStageRefPtr& stage) {
        std::vector<AreaLight> areaLightsDescriptors;
        std::vector<std::shared_ptr<rd::core::area_light>> area_lights;
        
        for (const auto& prim : stage->TraverseAll()) {
            if (prim.IsA<pxr::UsdLuxRectLight>()) {
                std::cout << "RectLight: " << prim.GetPath().GetString() << std::endl;
                pxr::GfMatrix4d xform = pxr::UsdGeomXformable(prim).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
                AreaLight light = extractAreaLightProperties(prim, xform);
                areaLightsDescriptors.push_back(light);
            }
        }
        for(const auto& light : areaLightsDescriptors) {
            auto light_material = make_shared<rd::core::light>(color(1.0, 1.0, 1.0), light.intensity);
            shared_ptr<rd::core::area_light> light_quad = make_shared<rd::core::area_light>(light.Q, light.u, light.v, light_material);
            area_lights.push_back(light_quad);
        }
        
        return area_lights;
    }
}

#endif