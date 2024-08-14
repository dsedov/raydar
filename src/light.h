#ifndef LIGHT_H
#define LIGHT_H

#include "raydar.h"
#include "data/ray.h"
#include "data/hittable.h"
#include "mis_material.h"

class area_light : public mis_hittable {
public:
    area_light(const point3& corner, const vec3& u, const vec3& v, const color& emit)
        : corner(corner), u(u), v(v), emission(emit), 
          emissive_mat(make_shared<emissive_material>(emit)) {
        normal = unit_vector(cross(u, v));
        area = u.length() * v.length();
        bbox = aabb(corner, corner + u + v);
        bbox.pad(0.001);
    }

    bool hit(const ray& r, interval ray_t, mis_hit_record& rec) const override {
        double denom = dot(normal, r.direction());
        if (fabs(denom) < 1e-8) return false; // Ray is parallel to the plane

        double t = dot(corner - r.origin(), normal) / denom;
        if (!ray_t.contains(t)) return false;

        point3 p = r.at(t);
        vec3 local_pos = p - corner;
        double alpha = dot(cross(v, local_pos), normal) / area;
        double beta = dot(cross(local_pos, u), normal) / area;

        if (alpha < 0 || alpha > 1 || beta < 0 || beta > 1) return false;

        rec.t = t;
        rec.p = p;
        rec.set_face_normal(r, normal);
        rec.mat = emissive_mat;
        rec.u = alpha;
        rec.v = beta;

        return true;
    }

    aabb bounding_box() const override {
        return bbox;
    }

    bool sample(const point3& origin, vec3& direction, double& distance, double& pdf_val, color& emit) const {
        point3 light_point = corner + random_double() * u + random_double() * v;
        direction = light_point - origin;
        distance = direction.length();
        direction = unit_vector(direction);

        double cos_theta = fabs(dot(normal, -direction));
        pdf_val = (distance * distance) / (area * cos_theta);
        emit = emission;

        return true;
    }

    double pdf(const point3& origin, const vec3& direction) const {
        mis_hit_record rec;
        if (!this->hit(ray(origin, direction, 0), interval(0.001, infinity), rec))
            return 0;

        double distance_squared = rec.t * rec.t * direction.length_squared();
        double cos_theta = fabs(dot(normal, -direction));
        return distance_squared / (area * cos_theta);
    }

    color power() const {
        return emission * area;
    }

private:
    point3 corner;
    vec3 u, v;
    vec3 normal;
    double area;
    color emission;
    shared_ptr<mis_material> emissive_mat;
    aabb bbox;
};
#endif