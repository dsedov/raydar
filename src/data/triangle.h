#ifndef TRIANGLE_H
#define TRIANGLE_H

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
class triangle {
public:
    triangle(const point3& v0, const point3& v1, const point3& v2, shared_ptr<rd::core::material> mat)
        : v0(v0), v1(v1), v2(v2), mat(mat) {
        edge1 = v1 - v0;
        edge2 = v2 - v0;
        normal = unit_vector( cross(edge1, edge2) );
    }
    aabb bounding_box() const {
        return aabb(v0, v1, v2);
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
    shared_ptr<rd::core::material> mat;
};
#endif