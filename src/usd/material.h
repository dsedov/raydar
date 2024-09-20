

#ifndef RD_USD_MATERIAL_H
#define RD_USD_MATERIAL_H

#include <cstdio>
#include <vector>
#include "../raydar.h"
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

#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/connectableAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/usd/usdLux/rectLight.h>

namespace rd::usd::material {


  

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

    pxr::UsdShadeShader GetShader(const pxr::UsdShadeMaterial& usdMaterial)
    {
        // First, try to get the shader using ComputeSurfaceSource (for UsdPreviewSurface)
        pxr::UsdShadeShader shader = usdMaterial.ComputeSurfaceSource();
        
        if (shader) {
            std::cout << "UsdPreviewSurface Shader: " << shader.GetPath().GetString() << std::endl;
            return shader;
        }
        
        // If ComputeSurfaceSource fails, try to get MaterialX shader
        pxr::UsdShadeOutput surfaceOutput = usdMaterial.GetSurfaceOutput(pxr::TfToken("mtlx:surface"));
        
        if (surfaceOutput) {
            pxr::UsdShadeConnectableAPI source;
            pxr::TfToken sourceName;
            pxr::UsdShadeAttributeType sourceType;
            
            if (pxr::UsdShadeConnectableAPI::GetConnectedSource(surfaceOutput, &source, &sourceName, &sourceType)) {
                shader = pxr::UsdShadeShader(source.GetPrim());
                if (shader) {
                    std::cout << "MaterialX Shader: " << shader.GetPath().GetString() << std::endl;
                    return shader;
                }
            }
        }
        
        // If no shader is found, return an invalid shader
        std::cout << "No shader found for the material." << std::endl;
        return pxr::UsdShadeShader();
    }

    std::unordered_map<std::string, std::shared_ptr<rd::core::material>> load_materials_from_stage(const pxr::UsdStageRefPtr& stage) {

        std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials;

        std::vector<pxr::UsdShadeMaterial> usdMaterials;
        findMaterialsRecursive(stage->GetPseudoRoot(), usdMaterials);

        for (const auto& usdMaterial : usdMaterials) {
            std::cout << "\nLoading Material: " << usdMaterial.GetPath().GetString() << std::endl;
            float base_weight = 1.0f;
            pxr::GfVec3f baseColor(1.0f, 0.0f, 0.0f); 

            float metalness = 0.0f;
            float transmission = 0.0f;
            float specular = 0.0f;
            float specular_roughness = 0.0f;
            float emission = 0.0f;
            pxr::GfVec3f emission_color(0.0f, 0.0f, 0.0f);
            // get shader
            pxr::UsdShadeShader shader = usdMaterial.ComputeSurfaceSource();
            if (shader) {
                std::cout << "Shader: " << shader.GetPath().GetString() << std::endl;

                // base weight
                pxr::UsdShadeInput baseWeightInput = shader.GetInput(pxr::TfToken("base"));
                if (baseWeightInput) {
                    if (baseWeightInput.Get(&base_weight)) {
                        std::cout << "Successfully read base weight: " << base_weight << std::endl;
                    } else {
                        std::cout << "Failed to read base weight, using default" << std::endl;
                        base_weight = 1.0f;
                    }
                }
                // Get base_color
                pxr::UsdShadeInput baseColorInput = shader.GetInput(pxr::TfToken("base_color"));
                if (baseColorInput) {
                    // Read the value of the input
                    
                    if (baseColorInput.Get(&baseColor)) {
                        std::cout << "Successfully read base color: " << baseColor[0] << ", " << baseColor[1] << ", " << baseColor[2] << std::endl;
                    } else {
                        std::cout << "Failed to read base color, using default" << std::endl;
                        baseColor = pxr::GfVec3f(1.0f, 0.0f, 0.0f);
                    }
                    color diffuseColor(baseColor[0], baseColor[1], baseColor[2]);
                    std::cout << "Base Color: " << baseColor[0] << ", " << baseColor[1] << ", " << baseColor[2] << std::endl;
                }
                // Get metalness
                pxr::UsdShadeInput metalnessInput = shader.GetInput(pxr::TfToken("metalness"));
                if (metalnessInput) {
                    if (metalnessInput.Get(&metalness)) {
                        std::cout << "Successfully read metalness: " << metalness << std::endl;
                    } else {
                        std::cout << "Failed to read metalness, using default" << std::endl;
                        metalness = 0.0f;
                    }
                }
                // Get transmission
                pxr::UsdShadeInput transmissionInput = shader.GetInput(pxr::TfToken("transmission"));
                if (transmissionInput) {
                    if (transmissionInput.Get(&transmission)) {
                        std::cout << "Successfully read transmission: " << transmission << std::endl;
                    } else {
                        std::cout << "Failed to read transmission, using default" << std::endl;
                        transmission = 0.0f;
                    }
                }
                // Get specular
                pxr::UsdShadeInput specularInput = shader.GetInput(pxr::TfToken("specular"));
                if (specularInput) {
                    if (specularInput.Get(&specular)) {
                        std::cout << "Successfully read specular: " << specular << std::endl;
                    } else {
                        std::cout << "Failed to read specular, using default" << std::endl;
                        specular = 0.0f;
                    }
                }
                // Get specular roughness
                pxr::UsdShadeInput specularRoughnessInput = shader.GetInput(pxr::TfToken("specular_roughness"));
                if (specularRoughnessInput) {
                    if (specularRoughnessInput.Get(&specular_roughness)) {
                        std::cout << "Successfully read specular roughness: " << specular_roughness << std::endl;
                    } else {
                        std::cout << "Failed to read specular roughness, using default" << std::endl;
                        specular_roughness = 0.0f;
                    }
                }
                // Get emission
                pxr::UsdShadeInput emissionInput = shader.GetInput(pxr::TfToken("emission"));
                if (emissionInput) {
                    if (emissionInput.Get(&emission)) {
                        std::cout << "Successfully read emission: " << emission << std::endl;
                    } else {
                        std::cout << "Failed to read emission, using default" << std::endl;
                        emission = 0.0f;
                    }
                }
                // Get emission color
                pxr::UsdShadeInput emissionColorInput = shader.GetInput(pxr::TfToken("emission_color"));
                if (emissionColorInput) {
                    // Read the value of the input
                    
                    if (emissionColorInput.Get(&emission_color)) {
                        std::cout << "Successfully read emission color: " << emission_color[0] << ", " << emission_color[1] << ", " << emission_color[2] << std::endl;
                    } else {
                        std::cout << "Failed to read emission color, using default" << std::endl;
                        emission_color = pxr::GfVec3f(0.0f, 0.0f, 0.0f);
                    }
                }

            } else {
                std::cout << "No shader found for material: " << usdMaterial.GetPath().GetString() << std::endl;
            }

            
            color diffuseColor(baseColor[0], baseColor[1], baseColor[2]);
            color emissionColor(emission_color[0], emission_color[1], emission_color[2]);
            std::shared_ptr<rd::core::advanced_pbr_material> mat = std::make_shared<rd::core::advanced_pbr_material>(
                base_weight, diffuseColor, metalness,  // base_weight, base_color, base_metalness
                specular, color(1,1,1), specular_roughness, 1.5,  // specular_weight, specular_color, specular_roughness, specular_ior
                transmission, color(1,1,1),  // transmission_weight, transmission_color
                emission, emissionColor  // emission_luminance, emission_color
            );
            materials[usdMaterial.GetPath().GetString()] = mat;
        }

        return materials;
    }

}
#endif