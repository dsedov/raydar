#include "../usd/light.h"
namespace rd::usd::light {
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

        pxr::UsdAttribute textureFileAttr = prim.GetAttribute(pxr::TfToken("inputs:texture:file"));
        if (textureFileAttr) {
            pxr::SdfAssetPath texturePath;
            if (textureFileAttr.Get(&texturePath)) {
                light.textureFilePath = texturePath.GetAssetPath();
            }
        }
        
        pxr::UsdAttribute verticesAttr = prim.GetAttribute(pxr::TfToken("primvars:arnold:vertices"));
        if (verticesAttr) {verticesAttr.Get(&light.vertices);
            for(auto& v : light.vertices) {
                std::cout << "Vertex: " << v << std::endl;
                v = transform.Transform(v);
                std::cout << "Vertex: " << v << std::endl;
            }
        }
        else {
            light.vertices = {{light.width/2, -light.height/2, 0}, {-light.width/2, -light.height/2, 0}, {-light.width/2, light.height/2, 0}, {light.width/2, light.height/2, 0}};
            for(auto& v : light.vertices) {
                std::cout << "Vertex: " << v << std::endl;
                v = transform.Transform(v);
                std::cout << "Vertex: " << v << std::endl;
            }
        }

        light.Q = point3(light.vertices[0][0], light.vertices[0][1], light.vertices[0][2]);
        light.u = vec3(light.vertices[1][0] - light.vertices[0][0],
                       light.vertices[1][1] - light.vertices[0][1],
                       light.vertices[1][2] - light.vertices[0][2]);
        light.v = vec3(light.vertices[3][0] - light.vertices[0][0],
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
    std::vector<rd::core::area_light*> extractAreaLightsFromUsdStage(const pxr::UsdStageRefPtr& stage, observer * observer) {
        std::vector<AreaLight> areaLightsDescriptors;
        std::vector<rd::core::area_light*> area_lights;
        
        for (const auto& prim : stage->TraverseAll()) {
            if (prim.IsA<pxr::UsdLuxRectLight>()) {
                std::cout << "RectLight: " << prim.GetPath().GetString() << std::endl;
                pxr::GfMatrix4d xform = pxr::UsdGeomXformable(prim).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
                AreaLight light = extractAreaLightProperties(prim, xform);
                areaLightsDescriptors.push_back(light);
            }
        }
        for(const auto& light : areaLightsDescriptors) {
            if (light.textureFilePath != "") {
                std::cout << "Loading texture: " << light.textureFilePath << std::endl;
                auto texture_ptr = new ImagePNG(ImagePNG::load(light.textureFilePath.c_str(), observer));

                auto light_material = new rd::core::light(spectrum::d65(), light.intensity, texture_ptr);
                auto light_quad = new rd::core::area_light(light.Q, light.u, light.v, light_material);
                area_lights.push_back(light_quad);
            }
            else {
                auto light_material = new rd::core::light(spectrum::d65(), light.intensity);
                auto light_quad = new rd::core::area_light(light.Q, light.u, light.v, light_material);
                area_lights.push_back(light_quad);
            }
            
        }
        
        return area_lights;
    }
}