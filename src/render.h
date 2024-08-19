#ifndef CAMERA_H
#define CAMERA_H

#include "raydar.h"
#include "data/interval.h"
#include "data/hittable.h"
#include "image/image.h"
#include "core/material.h"
#include "core/camera.h"
#include <thread>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>
#include <random>
#include <algorithm>
#include <unordered_map>
#include <queue>
#include <condition_variable>

class ProgressBar {
public:
    ProgressBar(int total) : total(total), last_printed(0) {}
    
    void update(int current) {
        int last = last_printed.load();
        if (current - last > total / 100) {  // Update every 1% progress
            if (last_printed.compare_exchange_strong(last, current)) {
                print(current);
            }
        }
    }

private:
    void print(int current) {
        float progress = static_cast<float>(current) / total;
        int barWidth = 70;
        std::cout << "\r[";
        int pos = barWidth * progress;
        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) std::cout << "=";
            else if (i == pos) std::cout << ">";
            else std::cout << " ";
        }
        std::cout << "] " << int(progress * 100.0) << "%" << std::flush;
    }

    int total;
    std::atomic<int> last_printed;
};


const int BUCKET_SIZE = 32; // Adjust this value as needed

struct Bucket {
    int start_x, start_y, end_x, end_y;
};
class GridCell {
public:
    aabb bounds;
    std::vector<const hittable*> objects;
    bool has_objects() const { return !objects.empty(); }
};

class MultiLevelGrid {
public:
    MultiLevelGrid(const hittable& world, int resolution = 32)
        : world_(world), resolution_(resolution) {
        build_grid();
    }

    bool trace(const ray& r, interval ray_t, hit_record& rec) const {
        vec3 inv_dir(1.0/r.direction().x(), 1.0/r.direction().y(), 1.0/r.direction().z());
        std::array<int, 3> step;
        point3 cell_idx;
        
        for (int i = 0; i < 3; ++i) {
            step[i] = inv_dir[i] >= 0 ? 1 : -1;
            cell_idx[i] = std::clamp(static_cast<int>((r.origin()[i] - grid_bounds_.min()[i]) * inv_cell_size_[i]), 0, resolution_ - 1);
        }

        while (true) {
            const GridCell& cell = grid_[cell_idx.x() + cell_idx.y() * resolution_ + cell_idx.z() * resolution_ * resolution_];
            
            if (cell.has_objects() && trace_cell(r, ray_t, rec, cell)) {
                return true;
            }

            int next_axis = get_next_axis(r.origin(), inv_dir, cell_idx, step);
            if (next_axis == -1) break;  // Ray has left the grid

            cell_idx[next_axis] += step[next_axis];
            if (cell_idx[next_axis] < 0 || cell_idx[next_axis] >= resolution_) break;
        }

        return false;
    }

private:
    const hittable& world_;
    int resolution_;
    std::vector<GridCell> grid_;
    aabb grid_bounds_;
    vec3 inv_cell_size_;

    void build_grid() {
        grid_bounds_ = world_.bounding_box();
        grid_bounds_.pad(0.001);  // Slightly expand the bounds to ensure all objects are inside

        vec3 cell_size = grid_bounds_.size() / resolution_;
        inv_cell_size_ = vec3(1.0 / cell_size.x(), 1.0 / cell_size.y(), 1.0 / cell_size.z());

        grid_.resize(resolution_ * resolution_ * resolution_);

        for (int i = 0; i < resolution_; ++i) {
            for (int j = 0; j < resolution_; ++j) {
                for (int k = 0; k < resolution_; ++k) {
                    point3 cell_min = grid_bounds_.min() + vec3(i, j, k) * cell_size;
                    aabb cell_bounds(cell_min, cell_min + cell_size);
                    
                    GridCell& cell = grid_[i + j * resolution_ + k * resolution_ * resolution_];
                    cell.bounds = cell_bounds;

                    if (boxes_overlap(cell_bounds, world_.bounding_box())) {
                        cell.objects.push_back(&world_);
                    }
                }
            }
        }
    }

    bool trace_cell(const ray& r, interval ray_t, hit_record& rec, const GridCell& cell) const {
        bool hit_anything = false;
        auto closest_so_far = ray_t.max;

        for (const auto* obj : cell.objects) {
            if (obj->hit(r, interval(ray_t.min, closest_so_far), rec)) {
                hit_anything = true;
                closest_so_far = rec.t;
            }
        }

        return hit_anything;
    }

    int get_next_axis(const point3& origin, const vec3& inv_dir, const point3& cell_idx, const std::array<int, 3>& step) const {
        vec3 t_max;
        for (int i = 0; i < 3; ++i) {
            if (step[i] > 0) {
                t_max[i] = ((cell_idx[i] + 1) * inv_cell_size_[i] + grid_bounds_.min()[i] - origin[i]) * inv_dir[i];
            } else {
                t_max[i] = (cell_idx[i] * inv_cell_size_[i] + grid_bounds_.min()[i] - origin[i]) * inv_dir[i];
            }
        }

        int next_axis = (t_max.x() < t_max.y()) ? ((t_max.x() < t_max.z()) ? 0 : 2) : ((t_max.y() < t_max.z()) ? 1 : 2);
        return (t_max[next_axis] >= 0) ? next_axis : -1;
    }

    bool boxes_overlap(const aabb& box1, const aabb& box2) const {
        return (box1.x.min <= box2.x.max && box1.x.max >= box2.x.min) &&
               (box1.y.min <= box2.y.max && box1.y.max >= box2.y.min) &&
               (box1.z.min <= box2.z.max && box1.z.max >= box2.z.min);
    }
};
class render {
public:

    int samples_per_pixel = 64;
    int max_depth         = 10;
    rd::core::camera camera;
    
    /* Public Camera Parameters Here */
    render(Image & image_buffer, rd::core::camera camera) : image_buffer(image_buffer), camera(camera) { }
    int mtpool_bucket_prog_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();
        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(num_threads);
        std::atomic<int> next_bucket(0);
        std::atomic<int> buckets_completed(0);
        std::atomic<bool> done(false);
        const int total_samples = sqrt_spp * sqrt_spp;
        ProgressBar progress_bar(total_samples);
        

        // Calculate total buckets
        const int total_buckets = ((image_buffer.width() + BUCKET_SIZE - 1) / BUCKET_SIZE) *
                                ((image_buffer.height() + BUCKET_SIZE - 1) / BUCKET_SIZE);
        MultiLevelGrid grid(world, 32);

        auto worker = [&]() {

            while (true) {
                int bucket_index = next_bucket.fetch_add(1);
                if (bucket_index >= total_buckets) {
                    break;
                }

                int start_x = (bucket_index % (image_buffer.width() / BUCKET_SIZE)) * BUCKET_SIZE;
                int start_y = (bucket_index / (image_buffer.width() / BUCKET_SIZE)) * BUCKET_SIZE;
                Bucket bucket{start_x, start_y, 
                            std::min(start_x + BUCKET_SIZE, image_buffer.width()), 
                            std::min(start_y + BUCKET_SIZE, image_buffer.height())};

                process_bucket(bucket, grid);

                int completed = buckets_completed.fetch_add(1) + 1;
                int current_progress = completed * total_samples / total_buckets;
                progress_bar.update(current_progress);
            }
        };

        // Start worker threads
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread(worker);
        }

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << std::endl;
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "Rendering time: " << duration.count() << " seconds" << std::endl;
        return duration.count();
    }
    
  private:
    /* Private Camera Variables Here */
    Image & image_buffer;
    vec3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    int    sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp
    vec3 u, v, w;              // Camera frame basis vectors
    color background_color = color(0.0, 0.0, 0.0) ;
    double pixel_samples_scale;

    void initialize(bool is_vertical_fov = false, bool fov_in_degrees = true) {
        auto focal_length = (camera.center - camera.look_at).length();
        
        // Convert FOV to radians if it's in degrees
        double fov_radians = fov_in_degrees ? degrees_to_radians(camera.fov) : camera.fov;
        
        // Calculate viewport dimensions based on FOV type
        double viewport_height, viewport_width;
        if (is_vertical_fov) {
            viewport_height = 2.0 * focal_length * tan(fov_radians / 2.0);
            viewport_width = viewport_height * (double(image_buffer.width()) / image_buffer.height());
        } else {
            viewport_width = 2.0 * focal_length * tan(fov_radians / 2.0);
            viewport_height = viewport_width / (double(image_buffer.width()) / image_buffer.height());
        }


        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(camera.center - camera.look_at);
        u = unit_vector(cross(camera.look_up, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_buffer.width();
        pixel_delta_v = viewport_v / image_buffer.height();

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = camera.center - (focal_length * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);


    }
    ray get_ray(int i, int j, int s_i, int s_j, int depth) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.


        auto offset = sample_square_stratified(s_i, s_j);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = camera.center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction, depth);
    }
    vec3 sample_square_stratified(int s_i, int s_j) const {

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }

    vec3 sample_square(int s_i, int s_j) const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double(), random_double() - 0.5, 0);
    }

    void process_bucket(const Bucket& bucket, const MultiLevelGrid& multi_level_grid) {
        const int PACKET_SIZE = 4; // Process 4 rays at a time
        const int total_samples = sqrt_spp * sqrt_spp;

        for (int j = bucket.start_y; j < bucket.end_y; j += PACKET_SIZE) {
            for (int i = bucket.start_x; i < bucket.end_x; i += PACKET_SIZE) {
                std::array<color, PACKET_SIZE * PACKET_SIZE> pixel_colors;
                pixel_colors.fill(color(0, 0, 0));

                for (int s = 0; s < total_samples; ++s) {
                    int s_i = s % sqrt_spp;
                    int s_j = s / sqrt_spp;

                    std::array<ray, PACKET_SIZE * PACKET_SIZE> rays;
                    for (int pj = 0; pj < PACKET_SIZE && j + pj < bucket.end_y; ++pj) {
                        for (int pi = 0; pi < PACKET_SIZE && i + pi < bucket.end_x; ++pi) {
                            rays[pj * PACKET_SIZE + pi] = get_ray(i + pi, j + pj, s_i, s_j, 0);
                        }
                    }

                    trace_packet(rays, multi_level_grid, pixel_colors);
                }

                // Set pixel colors
                for (int pj = 0; pj < PACKET_SIZE && j + pj < bucket.end_y; ++pj) {
                    for (int pi = 0; pi < PACKET_SIZE && i + pi < bucket.end_x; ++pi) {
                        image_buffer.set_pixel(i + pi, j + pj, pixel_colors[pj * PACKET_SIZE + pi] * pixel_samples_scale);
                    }
                }
            }
        }
    }

    void trace_packet(const std::array<ray, 16>& rays, const MultiLevelGrid& multi_level_grid, std::array<color, 16>& pixel_colors) {
        for (int i = 0; i < rays.size(); ++i) {
            pixel_colors[i] += ray_color(rays[i], max_depth, multi_level_grid);
        }
    }

    color ray_color(const ray& r, int depth, const MultiLevelGrid& multi_level_grid) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        bool hit = multi_level_grid.trace(r, interval(0.001, infinity), rec);

        if (!hit) {
            return background_color;
        }

        if (depth == max_depth && !rec.mat->is_visible()) {
            // Continue ray in the same direction with a small bias
            const double bias = 0.0001;
            ray continued_ray(rec.p + bias * r.direction(), r.direction(), r.get_depth());
            return ray_color(continued_ray, depth, multi_level_grid);
        }

        ray scattered;
        color attenuation;
        color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);
        double pdf_val; 
        if (!rec.mat->scatter(r, rec, attenuation, scattered, pdf_val)) {
            return emitted;
        }
        double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);
        color scattered_color = ray_color(scattered, depth-1, multi_level_grid);
        
        color final_color = emitted;
        if (pdf_val > 0 && scattering_pdf > 0) {
            final_color += (attenuation * scattering_pdf * scattered_color) / pdf_val;
        }

        return final_color;
    }
};

#endif