#include "../usd/geo.h"
namespace rd::usd::geo {
    rd::core::mesh* loadFromUsdMesh(const pxr::UsdGeomMesh& usd_mesh, rd::core::material* mat, const pxr::GfMatrix4d& transform) {
        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;
        pxr::VtArray<pxr::GfVec3f> normals;
        std::vector<triangle> triangles;

        usd_mesh.GetPointsAttr().Get(&points);
        usd_mesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        usd_mesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);
        usd_mesh.GetNormalsAttr().Get(&normals);

        // Print count for points, normals, and vertices
        std::cout << "Number of points: " << points.size() << std::endl;
        std::cout << "Number of normals: " << normals.size() << std::endl;
        

        std::vector<point3> vertices;
        std::vector<vec3> vertex_normals;
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
        std::cout << "Number of vertices: " << vertices.size() << std::endl;
        // Transform normals
        pxr::GfMatrix4d normalTransform = transform.GetInverse().GetTranspose();
        for (const auto& normal : normals) {
            pxr::GfVec3d transformedNormal = normalTransform.TransformDir(pxr::GfVec3d(normal));
            vec3 normalizedNormal = unit_vector(vec3(transformedNormal[0], transformedNormal[1], transformedNormal[2]));
            vertex_normals.push_back(normalizedNormal);
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

                if (index < vertex_normals.size() && index + i < vertex_normals.size() && index + i + 1< vertex_normals.size()) {
                    triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2],
                                        vertex_normals[index], vertex_normals[index + i], vertex_normals[index + i + 1],
                                        mat);
                } else {
                    // Use face normal as fallback for all vertices
                    vec3 face_normal = unit_vector(cross(vertices[v1] - vertices[v0], vertices[v2] - vertices[v0]));
                    triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2],
                                        face_normal, face_normal, face_normal,
                                        mat);
                }
            }

            index += faceVertexCount;
        }
        return new rd::core::mesh(triangles);
    }
    // Recursive function to traverse the USD hierarchy and extract meshes
    void traverseStageAndExtractMeshes(const pxr::UsdPrim& prim, std::vector<rd::core::mesh*>& meshes, std::unordered_map<std::string, rd::core::material*>& materials) {
        
        if (prim.IsA<pxr::UsdGeomMesh>()) {
            std::cout << std::endl << "Prim Path: " << prim.GetPath().GetString() << std::endl;
            pxr::UsdGeomMesh usdMesh(prim);
            pxr::GfMatrix4d xform = pxr::UsdGeomXformable(usdMesh).ComputeLocalToWorldTransform(pxr::UsdTimeCode::Default());
            // Extract material name
            rd::core::material* mat = materials["error"];
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
    std::vector<rd::core::mesh*> extractMeshesFromUsdStage(const pxr::UsdStageRefPtr& stage, std::unordered_map<std::string, rd::core::material*>& materials) {
        std::vector<rd::core::mesh*> meshes;
        pxr::UsdPrim rootPrim = stage->GetPseudoRoot();
        traverseStageAndExtractMeshes(rootPrim, meshes, materials);
        return meshes;
    }
    

}