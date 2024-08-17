



#ifndef RD_USD_GEO_H
#define RD_USD_GEO_H

#include <cstdio>
#include <vector>
#include "../raydar.h"

#include "../data/hittable_list.h"
#include "../data/quad.h"
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
    std::shared_ptr<rd::core::mesh> loadFromUsdMesh(const pxr::UsdGeomMesh& usd_mesh, shared_ptr<rd::core::material> mat, const pxr::GfMatrix4d& transform) {
        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;
        std::vector<triangle> triangles;

        usd_mesh.GetPointsAttr().Get(&points);
        usd_mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        usd_mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        std::vector<point3> vertices;
        point3 min_point(infinity, infinity, infinity);
        point3 max_point(-infinity, -infinity, -infinity);

        for (const auto& point : points) {
            // Apply transformation to each point
            pxr::GfVec3d transformedPoint = transform.Transform(pxr::GfVec3d(point));
            point3 vertex(transformedPoint[0], transformedPoint[1], transformedPoint[2]);
            vertices.push_back(vertex);

            // Update bounding box
            for (int i = 0; i < 3; ++i) {
                min_point[i] = std::min(min_point[i], vertex[i]);
                max_point[i] = std::max(max_point[i], vertex[i]);
            }
        }
        size_t index = 0;
        for (int faceVertexCount : faceVertexCounts) {
            if (faceVertexCount < 3) {
                std::cerr << "Error: face with less than 3 vertices" << std::endl;
                index += faceVertexCount;
                continue;
            }

            // Triangulate the face if it has more than 3 vertices
            for (int i = 1; i < faceVertexCount - 1; ++i) {
                int v0 = faceVertexIndices[index];
                int v1 = faceVertexIndices[index + i];
                int v2 = faceVertexIndices[index + i + 1];

                if (v0 < 0 || v0 >= vertices.size() ||
                    v1 < 0 || v1 >= vertices.size() ||
                    v2 < 0 || v2 >= vertices.size()) {
                    std::cerr << "Error: invalid vertex index" << std::endl;
                    continue;
                }

                triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2], mat);
            }

            index += faceVertexCount;
        }
        return std::shared_ptr<rd::core::mesh>(new rd::core::mesh(triangles));
    }
    // Recursive function to traverse the USD hierarchy and extract meshes
    void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<std::shared_ptr<rd::core::mesh>>& meshes, std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials) {
        
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

            meshes.push_back(loadFromUsdMesh(usdMesh, mat, xform));
        }
        for (const auto& child : prim.GetChildren()) traverseStageAndExtractMeshes(child, meshes, materials);
    }
    std::vector<std::shared_ptr<rd::core::mesh>> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::unordered_map<std::string, std::shared_ptr<rd::core::material>> materials) {
        std::vector<std::shared_ptr<rd::core::mesh>> meshes;
        pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
        traverseStageAndExtractMeshes(rootPrim, meshes, materials);
        return meshes;
    }
    

}
#endif