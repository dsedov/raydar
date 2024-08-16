

#ifndef RD_USD_MATERIAL_H
#define RD_USD_MATERIAL_H

#include <cstdio>
#include <vector>
#include "../raydar.h"
#include "../material.h"

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

namespace rd::usd::material {

    struct MaterialProperties {
        std::string fullPath;
        float metallic = 0.0f;
        float roughness = 0.5f;
        pxr::GfVec3f diffuseColor = pxr::GfVec3f(0.8f, 0.8f, 0.8f);
        pxr::GfVec3f emissiveColor = pxr::GfVec3f(0.0f, 0.0f, 0.0f);
        float ior = 1.5f;
    };


    std::shared_ptr<rd::core::material> createMaterialFromProperties(const MaterialProperties& props) {
        // Implement this function based on your material system
        // For now, we'll return a lambertian material as a placeholder
        //return std::make_shared<constant>(color(props.diffuseColor[0], props.diffuseColor[1], props.diffuseColor[2]));
        return std::make_shared<rd::core::advanced_pbr_material>(
            0.6, // base_weight (guessed)
            color(props.diffuseColor[0], props.diffuseColor[1], props.diffuseColor[2]), // base_color
            props.metallic, // base_metalness

            1.0, // specular_weight (guessed)
            color(1.0, 1.0, 1.0), // specular_color (guessed)
            props.roughness, // specular_roughness
            props.ior, // specular_ior

            0.0, // transmission_weight (guessed)
            color(1.0, 1.0, 1.0), // transmission_color (guessed)

            1.0, // emission_luminance (guessed)
            color(props.emissiveColor[0], props.emissiveColor[1], props.emissiveColor[2]) // emission_color
        );
        return std::make_shared<rd::core::lambertian>(color(props.diffuseColor[0], props.diffuseColor[1], props.diffuseColor[2]));

        
    }

    pxr::UsdShadeShader findShaderInNodeGraph(const pxr::UsdPrim& nodeGraphPrim) {
        for (const auto& child : nodeGraphPrim.GetChildren()) {
            if (child.IsA<pxr::UsdShadeShader>()) {
                return pxr::UsdShadeShader(child);
            }
        }
        return pxr::UsdShadeShader();
    }
    void findMaterialsRecursive(const pxr::UsdPrim& prim, std::vector<pxr::UsdShadeMaterial>& materials) {
        if (prim.IsA<pxr::UsdShadeMaterial>()) {
            materials.push_back(pxr::UsdShadeMaterial(prim));
        }
        
        for (const auto& child : prim.GetChildren()) {
            findMaterialsRecursive(child, materials);
        }
    }
    MaterialProperties extractMaterialProperties(const pxr::UsdShadeMaterial& usdMaterial) {
        MaterialProperties props;
        props.fullPath = usdMaterial.GetPath().GetString();

        // Look for MaterialX and UsdPreviewSurface NodeGraphs
        pxr::UsdPrim materialXNodeGraph = usdMaterial.GetPrim().GetChild(pxr::TfToken("MaterialX"));
        pxr::UsdPrim previewSurfaceNodeGraph = usdMaterial.GetPrim().GetChild(pxr::TfToken("UsdPreviewSurface"));

        pxr::UsdShadeShader materialXShader, previewSurfaceShader;

        if (materialXNodeGraph) {
            materialXShader = findShaderInNodeGraph(materialXNodeGraph);
        }

        if (previewSurfaceNodeGraph) {
            previewSurfaceShader = findShaderInNodeGraph(previewSurfaceNodeGraph);
        }

        // Prefer MaterialX if available, otherwise use UsdPreviewSurface
        pxr::UsdShadeShader shaderToUse = materialXShader ? materialXShader : previewSurfaceShader;

        if (shaderToUse) {
            pxr::VtValue value;

            if (shaderToUse.GetInput(pxr::TfToken("metallic")).Get(&value)) {
                props.metallic = value.Get<float>();
            }
            if (shaderToUse.GetInput(pxr::TfToken("roughness")).Get(&value)) {
                props.roughness = value.Get<float>();
            }
            if (shaderToUse.GetInput(pxr::TfToken("diffuseColor")).Get(&value)) {
                props.diffuseColor = value.Get<pxr::GfVec3f>();
            }
            if (shaderToUse.GetInput(pxr::TfToken("emissiveColor")).Get(&value)) {
                props.emissiveColor = value.Get<pxr::GfVec3f>();
            }
            if (shaderToUse.GetInput(pxr::TfToken("ior")).Get(&value)) {
                props.ior = value.Get<float>();
            }
        }

        return props;
    }
        std::unordered_map<std::string, std::shared_ptr<rd::core::material>> loadMaterialsFromStage(const pxr::UsdStageRefPtr& stage) {
        std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials;

        std::vector<pxr::UsdShadeMaterial> usdMaterials;
        findMaterialsRecursive(stage->GetPseudoRoot(), usdMaterials);

        for (const auto& usdMaterial : usdMaterials) {
            MaterialProperties props = extractMaterialProperties(usdMaterial);
            std::shared_ptr<rd::core::material> mat = createMaterialFromProperties(props);
            materials[props.fullPath] = mat;

            std::cout << "Loaded material: " << props.fullPath << std::endl;
            std::cout << "  Metallic: " << props.metallic << std::endl;
            std::cout << "  Roughness: " << props.roughness << std::endl;
            std::cout << "  Diffuse Color: " << props.diffuseColor << std::endl;
            std::cout << "  Emissive Color: " << props.emissiveColor << std::endl;
            std::cout << "  IOR: " << props.ior << std::endl;
        }

        return materials;
    }

}
#endif