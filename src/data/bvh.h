#ifndef BVH_H
#define BVH_H

#include "../raydar.h"
#include "../core/mesh.h"
#include "triangle.h"
#include "aabb.h"
#include "hittable.h"
#include "hittable_list.h"

class bvh_node : public hittable {
  public:
    bvh_node(hittable_list list) : bvh_node(list.objects, 0, list.objects.size()) {
        // There's a C++ subtlety here. This constructor (without span indices) creates an
        // implicit copy of the hittable list, which we will modify. The lifetime of the copied
        // list only extends until this constructor exits. That's OK, because we only need to
        // persist the resulting bounding volume hierarchy.
    }

    bvh_node(std::vector<shared_ptr<hittable>>& objects, size_t start, size_t end) {
        int axis = random_int(0,2);

        auto comparator = (axis == 0) ? box_x_compare
                        : (axis == 1) ? box_y_compare
                                      : box_z_compare;

        size_t object_span = end - start;

        if (object_span == 1) {
            left = right = objects[start];
        } else if (object_span == 2) {
            left = objects[start];
            right = objects[start+1];
        } else {
            std::sort(std::begin(objects) + start, std::begin(objects) + end, comparator);

            auto mid = start + object_span/2;
            left = make_shared<bvh_node>(objects, start, mid);
            right = make_shared<bvh_node>(objects, mid, end);
        }

        bbox = aabb(left->bounding_box(), right->bounding_box());
    }
    // Split in the middle along the given axis. Return two new meshes with the split triangles.
    static std::vector<rd::core::mesh> & split(rd::core::mesh & mesh, std::vector<rd::core::mesh> & meshes, int level = 0) {
        // padded string 
        //std::string padding = std::string(level, ' ');
        if(mesh.triangles.size() == 0) {
            //std::cout << padding << "No triangles to split" << std::endl;
            return meshes;
        }
        if(mesh.triangles.size() <= 10) {
            //std::cout << padding << "Terminating mesh with " << triangles.size() << " triangles to meshes" << std::endl;
            meshes.push_back(mesh);
            return meshes;
        }
        //std::cout << padding << "Splitting mesh with " << triangles.size() << " triangles" << std::endl;

        // Find the axis that allows for the most equal split
        int best_axis = 0;
        double best_balance = std::numeric_limits<double>::max();
        double mid;

        for (int axis = 0; axis < 3; ++axis) {
            std::vector<double> centroids;
            for (const auto& triangle : mesh.triangles) {
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

        std::vector<triangle> left_triangles, right_triangles;

        // Sort triangles into left and right based on their center point
        for (const auto& triangle : mesh.triangles) {
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
            if (left_triangles.size() == mesh.triangles.size()) {
                //std::cout << padding << "Terminating mesh with " << left_triangles.size() << " triangles to meshes" << std::endl;
                meshes.push_back(mesh);
                return meshes;
            }
            rd::core::mesh left_mesh(left_triangles);
            meshes = split(left_mesh, meshes, level + 1);
        }
        if (right_triangles.size() > 0) {
            if (right_triangles.size() == mesh.triangles.size()) {
                //std::cout << padding << "Terminating mesh with " << right_triangles.size() << " triangles to meshes" << std::endl;
                meshes.push_back(mesh);
                return meshes;
            }
            rd::core::mesh right_mesh(right_triangles);
            meshes = split(right_mesh, meshes, level + 1);
        }
        return meshes;
    }
    bool hit(const ray& r, interval ray_t, hit_record& rec) const override {
        if (!bbox.hit(r, ray_t))
            return false;

        bool hit_left = left->hit(r, ray_t, rec);
        bool hit_right = right->hit(r, interval(ray_t.min, hit_left ? rec.t : ray_t.max), rec);

        return hit_left || hit_right;
    }

    aabb bounding_box() const override { return bbox; }

  private:
    shared_ptr<hittable> left;
    shared_ptr<hittable> right;
    aabb bbox;

    static bool box_compare(
        const shared_ptr<hittable> a, const shared_ptr<hittable> b, int axis_index
    ) {
        auto a_axis_interval = a->bounding_box().axis_interval(axis_index);
        auto b_axis_interval = b->bounding_box().axis_interval(axis_index);
        return a_axis_interval.min < b_axis_interval.min;
    }

    static bool box_x_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 0);
    }

    static bool box_y_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 1);
    }

    static bool box_z_compare (const shared_ptr<hittable> a, const shared_ptr<hittable> b) {
        return box_compare(a, b, 2);
    }
};

#endif