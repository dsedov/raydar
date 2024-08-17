#ifndef OBJ_H
#define OBJ_H

#include "../data/hittable.h"
#include "../data/interval.h"
#include "../data/vec3.h"
#include "../data/aabb.h"
#include "../data/triangle.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/base/vt/array.h>


namespace rd::core {
    class mesh : public hittable {
    public:
        mesh(std::vector<triangle> triangles){
            this->triangles = triangles;
            for (const auto& triangle : triangles) {
                bbox = aabb(bbox, triangle.bounding_box());
            }
            bbox.pad(0.000001);
        }
        int get_num_triangles() const {
            return triangles.size();
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
        aabb bounding_box() const override {
            return bbox;
        }
    std::vector<triangle> triangles;
    private:
        
        aabb bbox;
    };
}
#endif