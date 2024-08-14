#ifndef HITTABLE_H
#define HITTABLE_H

#include "ray.h"
#include "interval.h"
#include "aabb.h"

class material;
class mis_material;

class hit_record {
  public:
    point3 p;
    vec3 normal;
    bool front_face;
    shared_ptr<material> mat;
    double t;
    double u;
    double v;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};
class mis_hit_record {
  public:
    point3 p;
    vec3 normal;
    bool front_face;
    shared_ptr<mis_material> mat;
    double t;
    double u;
    double v;

    void set_face_normal(const ray& r, const vec3& outward_normal) {
        // Sets the hit record normal vector.
        // NOTE: the parameter `outward_normal` is assumed to have unit length.

        front_face = dot(r.direction(), outward_normal) < 0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

class hittable {
  public:
    virtual ~hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, hit_record& rec) const = 0;
    virtual aabb bounding_box() const = 0;
};

class mis_hittable {
  public:
    virtual ~mis_hittable() = default;

    virtual bool hit(const ray& r, interval ray_t, mis_hit_record& rec) const = 0;
    virtual aabb bounding_box() const = 0;
    // New methods for MIS
    virtual bool sample(const point3& origin, vec3& direction, double& distance, double& pdf_val, color& emit) const {
        return false;
    }

    virtual double pdf(const point3& origin, const vec3& direction) const {
        return 0.0;
    }

    virtual color power() const {
        return color(0, 0, 0);
    }

    // Helper method to determine if this hittable is a light source
    virtual bool is_light() const {
        return false;
    }
};

#endif