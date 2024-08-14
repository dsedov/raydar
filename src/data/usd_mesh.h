#ifndef OBJ_H
#define OBJ_H

#include "hittable.h"
#include "interval.h"
#include "vec3.h"
#include "aabb.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/base/vt/array.h>

class Triangle {
public:
    Triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<mis_material> mat)
        : v0(v0), v1(v1), v2(v2), mat(mat) {
        edge1 = v1 - v0;
        edge2 = v2 - v0;
        normal = unit_vector( cross(edge1, edge2) );
    }
    aabb bounding_box() const {
        return aabb(v0, v1, v2);
    }
    bool hit(const ray& r, interval ray_t, mis_hit_record& rec) const {
        const float EPSILON = 0.0000001;
        vec3 h = cross(r.direction(), edge2);
        float a = dot(edge1, h);

        if (a > -EPSILON && a < EPSILON)
            return false;    // Ray is parallel to triangle

        float f = 1.0 / a;
        vec3 s = r.origin() - v0;
        float u = f * dot(s, h);

        if (u < 0.0 || u > 1.0)
            return false;

        vec3 q = cross(s, edge1);
        float v = f * dot(r.direction(), q);

        if (v < 0.0 || u + v > 1.0)
            return false;

        // Compute t to find intersection point
        float t = f * dot(edge2, q);

        if (t > ray_t.min && t < ray_t.max) {
            rec.t = t;
            rec.p = r.at(t);
            rec.set_face_normal(r, normal);
            rec.mat = mat;
            return true;
        }

        return false;
    }
    void print() const {
        std::cout << "Triangle:\n"
                  << "  v0: " << v0 << "\n"
                  << "  v1: " << v1 << "\n"
                  << "  v2: " << v2 << "\n"
                  << "  normal: " << normal << "\n";
    }

private:
    point3 v0, v1, v2;
    vec3 edge1, edge2, normal;
    shared_ptr<mis_material> mat;
};

class usd_mesh : public mis_hittable {
public:
    usd_mesh(std::vector<Triangle> triangles, shared_ptr<mis_material> mat){
        this->triangles = triangles;
        this->mat = mat;
        for (const auto& triangle : triangles) {
            bbox = aabb(bbox, triangle.bounding_box());
        }
        // padd bbox by 0.01
        bbox.pad(0.000001);
    }
    usd_mesh(const pxr::UsdGeomMesh& usdMesh, shared_ptr<mis_material> mat, const pxr::GfMatrix4d& transform)
        : mat(mat), transform(transform) {
        //std::cout << "Transform: " << transform << std::endl;
        loadFromUsdMesh(usdMesh);
    }
    int get_num_triangles() const {
        return triangles.size();
    }
    // Split in the middle along the given axis. Return two new meshes with the split triangles.
    std::vector<usd_mesh> & split(std::vector<usd_mesh> & meshes, int level = 0) {
        // padded string 
        //std::string padding = std::string(level, ' ');
        if(triangles.size() == 0) {
            //std::cout << padding << "No triangles to split" << std::endl;
            return meshes;
        }
        if(triangles.size() <= 10) {
            //std::cout << padding << "Terminating mesh with " << triangles.size() << " triangles to meshes" << std::endl;
            meshes.push_back(*this);
            return meshes;
        }
        //std::cout << padding << "Splitting mesh with " << triangles.size() << " triangles" << std::endl;

        // Find the axis that allows for the most equal split
        int best_axis = 0;
        double best_balance = std::numeric_limits<double>::max();
        double mid;

        for (int axis = 0; axis < 3; ++axis) {
            std::vector<double> centroids;
            for (const auto& triangle : triangles) {
                aabb triangle_bbox = triangle.bounding_box();
                interval axis_interval = triangle_bbox.axis_interval(axis);
                centroids.push_back((axis_interval.min + axis_interval.max) / 2.0);
            }

            std::nth_element(centroids.begin(), centroids.begin() + centroids.size()/2, centroids.end());
            double median = centroids[centroids.size()/2];

            int left_count = 0, right_count = 0;
            for (double centroid : centroids) {
                if (centroid < median) ++left_count;
                else ++right_count;
            }

            double balance = std::abs(left_count - right_count);
            if (balance < best_balance) {
                best_balance = balance;
                best_axis = axis;
                mid = median;
            }
        }

        std::vector<Triangle> left_triangles, right_triangles;

        // Sort triangles into left and right based on their center point
        for (const auto& triangle : triangles) {
            aabb triangle_bbox = triangle.bounding_box();
            interval axis_interval = triangle_bbox.axis_interval(best_axis);
            double mid_point = (axis_interval.min + axis_interval.max) / 2.0;
            if (mid_point < mid) {
                left_triangles.push_back(triangle);
            } else {
                right_triangles.push_back(triangle);
            }
        }

        if (left_triangles.size() > 0) { 
            if (left_triangles.size() == triangles.size()) {
                //std::cout << padding << "Terminating mesh with " << left_triangles.size() << " triangles to meshes" << std::endl;
                meshes.push_back(*this);
                return meshes;
            }
            usd_mesh left_mesh(left_triangles, mat);
            meshes = left_mesh.split(meshes, level + 1);
        }
        if (right_triangles.size() > 0) {
            if (right_triangles.size() == triangles.size()) {
                //std::cout << padding << "Terminating mesh with " << right_triangles.size() << " triangles to meshes" << std::endl;
                meshes.push_back(*this);
                return meshes;
            }
            usd_mesh right_mesh(right_triangles, mat);
            meshes = right_mesh.split(meshes, level + 1);
        }
        return meshes;
    }

    bool hit(const ray& r, interval ray_t, mis_hit_record& rec) const override {
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto& triangle : triangles) {
            if (triangle.hit(r, interval(ray_t.min, closest_so_far), rec)) {
                hit_anything = true;
                closest_so_far = rec.t;
            }
        }

        return hit_anything;
    }

    aabb bounding_box() const override {
        return bbox;
    }

private:
    std::vector<Triangle> triangles;
    pxr::GfMatrix4d transform;
    aabb bbox;
    shared_ptr<mis_material> mat;

    void loadFromUsdMesh(const pxr::UsdGeomMesh& usdMesh) {
        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;

        usdMesh.GetPointsAttr().Get(&points);
        usdMesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        usdMesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

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

        // Set the bounding box
        bbox = aabb(min_point, max_point);

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

        //std::cout << "Loaded " << vertices.size() << " vertices and " << triangles.size() << " triangles from USD Mesh" << std::endl;
        //std::cout << "Bounding box: min(" << min_point << "), max(" << max_point << ")" << std::endl;
    }
};
#endif