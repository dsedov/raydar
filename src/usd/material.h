

#ifndef RD_USD_MATERIAL_H
#define RD_USD_MATERIAL_H

#include <cstdio>
#include <vector>
#include "../raydar.h"
#include "../core/material.h"

#include "../data/hittable.h"
#include "../data/hittable_list.h"
#include "../data/quad.h"
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



    std::unordered_map<std::string, std::shared_ptr<rd::core::material>> load_materials_from_stage(const pxr::UsdStageRefPtr& stage) {

        std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials;

        std::vector<pxr::UsdShadeMaterial> usdMaterials;
        findMaterialsRecursive(stage->GetPseudoRoot(), usdMaterials);

        for (const auto& usdMaterial : usdMaterials) {
            std::cout << "Loading Material: " << usdMaterial.GetPath().GetString() << std::endl;
            pxr::GfVec3f baseColor(1.0f, 0.0f, 0.0f); 
            // get shader
            pxr::UsdShadeShader shader = usdMaterial.ComputeSurfaceSource();
            if (shader) {
                std::cout << "Shader: " << shader.GetPath().GetString() << std::endl;
                // Get base_color
                pxr::UsdShadeInput baseColorInput = shader.GetInput(pxr::TfToken("base_color"));
                if (baseColorInput) {
                    // Read the value of the input
                    
                    if (baseColorInput.Get(&baseColor)) {
                        std::cout << "Successfully read base color" << std::endl;
                    } else {
                        std::cout << "Failed to read base color, using default" << std::endl;
                        baseColor = pxr::GfVec3f(1.0f, 0.0f, 0.0f);
                    }
                    color diffuseColor(baseColor[0], baseColor[1], baseColor[2]);
                    std::cout << "Base Color: " << baseColor[0] << ", " << baseColor[1] << ", " << baseColor[2] << std::endl;
                }
            }

            
            color diffuseColor(baseColor[0], baseColor[1], baseColor[2]);
            std::shared_ptr<rd::core::advanced_pbr_material> mat = std::make_shared<rd::core::advanced_pbr_material>(
                1.0, diffuseColor, 0.0,  // base_weight, base_color, base_metalness
                0.5, color(1,1,1), 0.5, 1.5,  // specular_weight, specular_color, specular_roughness, specular_ior
                0.0, color(1,1,1),  // transmission_weight, transmission_color
                0.0, color(0,0,0)  // emission_luminance, emission_color
            );
            materials[usdMaterial.GetPath().GetString()] = mat;
        }

        return materials;
    }

}
#endif