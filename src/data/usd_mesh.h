#ifndef OBJ_H
#define OBJ_H

#include "hittable.h"
#include "interval.h"

#include "vec3.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/base/vt/array.h>

#include "../nanort.h"

class usd_mesh : public hittable {
public:

    usd_mesh(const pxr::UsdGeomMesh& usdMesh, shared_ptr<material> mat, const pxr::GfMatrix4d& transform)
        : mat(mat), transform(transform) {
        loadFromUsdMesh(usdMesh);
        buildBVH();
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        nanort::Ray<float> ray;
        ray.min_t = ray_t.min;
        ray.max_t = ray_t.max;
        ray.org[0] = r.origin().x();
        ray.org[1] = r.origin().y();
        ray.org[2] = r.origin().z();
        ray.dir[0] = r.direction().x();
        ray.dir[1] = r.direction().y();
        ray.dir[2] = r.direction().z();
        nanort::TriangleIntersector<> triangle_intersecter(vertices.data(), indices.data(), sizeof(float) * 3);
        nanort::TriangleIntersection<> isect;
        nanort::BVHTraceOptions trace_options;
        bool hit = accel.Traverse(ray, triangle_intersecter, &isect, trace_options);
        return hit;
    }


private:

    pxr::GfMatrix4d transform;
    shared_ptr<material> mat;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;
    nanort::BVHAccel<float> accel;
    void loadFromUsdMesh(const pxr::UsdGeomMesh& usdMesh) {
        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;

        usdMesh.GetPointsAttr().Get(&points);
        usdMesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        usdMesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        point3 min_point(infinity, infinity, infinity);
        point3 max_point(-infinity, -infinity, -infinity);

        vertices.reserve(points.size() * 3);
        for (const auto& point : points) {
            pxr::GfVec3d transformedPoint = transform.Transform(pxr::GfVec3d(point));
            vertices.push_back(transformedPoint[0]);
            vertices.push_back(transformedPoint[1]);
            vertices.push_back(transformedPoint[2]);
        }
        for (const auto& faceVertexIndex : faceVertexIndices) {
            indices.push_back(faceVertexIndex);
        }

        indices.reserve(faceVertexIndices.size());
        size_t index = 0;
        for (int faceVertexCount : faceVertexCounts) {
            if (faceVertexCount < 3) {
                std::cerr << "Error: face with less than 3 vertices" << std::endl;
                index += faceVertexCount;
                continue;
            }

            for (int i = 1; i < faceVertexCount - 1; ++i) {
                indices.push_back(faceVertexIndices[index]);
                indices.push_back(faceVertexIndices[index + i]);
                indices.push_back(faceVertexIndices[index + i + 1]);
            }

            index += faceVertexCount;
        }
    }
    void buildBVH() {
        nanort::BVHBuildOptions<float> options;
        nanort::TriangleMesh<float> mesh(vertices.data(), indices.data(), sizeof(float) * 3);
        nanort::TriangleSAHPred<float> pred(vertices.data(), indices.data(), sizeof(float) * 3);
        bool built = accel.Build(indices.size() / 3, mesh, pred, options);
        if (!built) {
            std::cerr << "Error: failed to build BVH" << std::endl;
        } else {
            nanort::BVHBuildStatistics stats = accel.GetStatistics();
            std::cout << "BVH built with " << stats.num_branch_nodes << " nodes" << std::endl;
        }
    }
};
#endif