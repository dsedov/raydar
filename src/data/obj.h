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

class Triangle {
public:
    Triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<material> mat)
        : v0(v0), v1(v1), v2(v2), mat(mat) {
        edge1 = v1 - v0;
        edge2 = v2 - v0;
        normal = unit_vector( cross(edge1, edge2) );
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const {
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
    shared_ptr<material> mat;
};

class obj : public hittable {
public:
    obj(const std::string& filename, shared_ptr<material> mat) : mat(mat) {
        loadFromFile(filename);        
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
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

private:
    std::vector<Triangle> triangles;
    shared_ptr<material> mat;

    void loadFromFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filename);
        }

        std::vector<point3> vertices;
        std::string line;
        int line_number = 0;

        while (std::getline(file, line)) {
            line_number++;
            std::istringstream iss(line);
            std::string type;
            iss >> type;

            if (type == "v") {
                double x, y, z;
                if (!(iss >> x >> y >> z)) {
                    std::cerr << "Error parsing vertex on line " << line_number << ": " << line << std::endl;
                    continue;
                }
                vertices.emplace_back(x, y, z);
            } else if (type == "f") {
                std::vector<int> face_vertices;
                std::string vertex;
                while (iss >> vertex) {
                    std::istringstream vertex_iss(vertex);
                    int v;
                    if (vertex_iss >> v) {
                        face_vertices.push_back(v);
                    }
                }

                if (face_vertices.size() < 3) {
                    std::cerr << "Error: face with less than 3 vertices on line " << line_number << ": " << line << std::endl;
                    continue;
                }

                // Triangulate the face if it has more than 3 vertices
                for (size_t i = 1; i < face_vertices.size() - 1; ++i) {
                    int v0 = face_vertices[0] - 1;  // OBJ files use 1-based indexing
                    int v1 = face_vertices[i] - 1;
                    int v2 = face_vertices[i+1] - 1;

                    if (v0 < 0 || v0 >= vertices.size() ||
                        v1 < 0 || v1 >= vertices.size() ||
                        v2 < 0 || v2 >= vertices.size()) {
                        std::cerr << "Error: invalid vertex index on line " << line_number << ": " << line << std::endl;
                        continue;
                    }

                    triangles.emplace_back(vertices[v0], vertices[v1], vertices[v2], mat);
                }
            }
        }

        std::cout << "Loaded " << vertices.size() << " vertices and " << triangles.size() << " triangles from " << filename << std::endl;
    }
};
class mesh : public hittable {
public:
    mesh(const pxr::UsdGeomMesh& usdMesh, shared_ptr<material> mat, const pxr::GfMatrix4d& transform)
        : mat(mat), transform(transform) {
        loadFromUsdMesh(usdMesh);
    }

    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
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

private:
    std::vector<Triangle> triangles;
    pxr::GfMatrix4d transform;
    shared_ptr<material> mat;

    void loadFromUsdMesh(const pxr::UsdGeomMesh& usdMesh) {
        pxr::VtArray<pxr::GfVec3f> points;
        pxr::VtArray<int> faceVertexCounts;
        pxr::VtArray<int> faceVertexIndices;

        usdMesh.GetPointsAttr().Get(&points);
        usdMesh.GetFaceVertexCountsAttr().Get(&faceVertexCounts);
        usdMesh.GetFaceVertexIndicesAttr().Get(&faceVertexIndices);

        std::vector<point3> vertices;
        for (const auto& point : points) {
            // Apply transformation to each point
            pxr::GfVec3d transformedPoint = transform.Transform(pxr::GfVec3d(point));
            vertices.emplace_back(transformedPoint[0], transformedPoint[1], transformedPoint[2]);
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

        std::cout << "Loaded " << vertices.size() << " vertices and " << triangles.size() << " triangles from USD Mesh" << std::endl;
    }
};
#endif