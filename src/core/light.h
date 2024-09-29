#ifndef RD_CORE_LIGHT_H
#define RD_CORE_LIGHT_H

#include "../data/hittable.h"
#include "../data/aabb.h"
#include "../data/interval.h"
#include "../data/ray.h"
#include "../data/vec3.h"
#include "../core/material.h"

namespace rd::core {
    class area_light : public hittable {
        public:
            area_light(const point3& Q, const vec3& u, const vec3& v, rd::core::material* mat)
            : Q(Q), u(u), v(v), mat(mat)
            {
                auto n = cross(u, v);
                normal = unit_vector(n);
                D = dot(normal, Q);
                w = n / dot(n,n);
                area = n.length();
                set_bounding_box();
            }

            virtual void set_bounding_box() {
                // Compute the bounding box of all four vertices.
                auto bbox_diagonal1 = aabb(Q, Q + u + v);
                auto bbox_diagonal2 = aabb(Q + u, Q + v);
                bbox = aabb(bbox_diagonal1, bbox_diagonal2);
                bbox.pad(0.0001);

            }

            aabb bounding_box() const override { return bbox; }

            bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
                auto denom = dot(normal, r.direction());

                // No hit if the ray is parallel to the plane.
                if (std::fabs(denom) < 1e-8)
                    return false;

                // Return false if the hit point parameter t is outside the ray interval.
                auto t = (D - dot(normal, r.origin())) / denom;
                if (!ray_t.contains(t))
                    return false;

                // Determine if the hit point lies within the planar shape using its plane coordinates.
                auto intersection = r.at(t);
                vec3 planar_hitpt_vector = intersection - Q;
                auto alpha = dot(w, cross(planar_hitpt_vector, v));
                auto beta = dot(w, cross(u, planar_hitpt_vector));

                if (!is_interior(alpha, beta, rec))
                    return false;

                // Ray hits the 2D shape; set the rest of the hit record and return true.

                rec.t = t;
                rec.p = intersection;
                rec.mat = mat;
                rec.set_face_normal(r, normal);

                return true;
            }
            double pdf_value(const point3& origin, const vec3& direction) const override {
                hit_record rec;
                if (!this->hit(ray(origin, direction), interval(0.001, infinity), rec))
                    return 0;

                auto distance_squared = rec.t * rec.t * direction.length_squared();
                auto cosine = std::fabs(dot(direction, rec.normal) / direction.length());

                return distance_squared / (cosine * area);
            }

            vec3 random(const point3& origin) const override {
                auto p = Q + (random_double() * u) + (random_double() * v);
                return p - origin;
            }
            virtual bool is_interior(double a, double b, hit_record& rec) const {
                interval unit_interval = interval(0, 1);
                // Given the hit point in plane coordinates, return false if it is outside the
                // primitive, otherwise set the hit record UV coordinates and return true.

                if (!unit_interval.contains(a) || !unit_interval.contains(b))
                    return false;

                rec.u = a;
                rec.v = b;
                return true;
            }
            void set_emission(const spectrum& c){
                if (auto light_mat = dynamic_cast<rd::core::light*>(mat)) {
                    light_mat->set_emission(c);
                }
            }
        private:
            point3 Q;
            vec3 u, v;
            vec3 w;
            rd::core::material * mat;
            aabb bbox;
            vec3 normal;
            double D;
            double area;
    };
}

#endif