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

class render {
public:

    int samples_per_pixel = 64;
    int max_depth         = 10;
    rd::core::camera camera;
    
    /* Public Camera Parameters Here */
    render(Image & image_buffer, rd::core::camera camera) : image_buffer(image_buffer), camera(camera) { }
    int mtpool_bucket_prog_render(const hittable& world, const hittable& lights) {
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

                process_bucket(bucket, world, lights);

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
        //vec3 offset = dithering.sample_square_dithered(s_i, s_j, i, j, sqrt_spp);
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

    void process_bucket(const Bucket& bucket, const hittable& world, const hittable& lights) {
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

                    trace_packet(rays, world, lights, pixel_colors);
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

    void trace_packet(const std::array<ray, 16>& rays, const hittable& world, const hittable& lights, std::array<color, 16>& pixel_colors) {
        for (int i = 0; i < rays.size(); ++i) {
            pixel_colors[i] += ray_color(rays[i], max_depth, world, lights);
        }
    }

    color ray_color(const ray& r, int depth, const hittable& world, const hittable& lights) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        bool hit = world.hit(r, interval(0.001, infinity), rec);

        if (!hit) {
            return background_color;
        }

        if (!rec.mat->is_visible()) {
            // Continue ray in the same direction with a small bias
            const double bias = 0.0001;
            ray continued_ray(rec.p + bias * r.direction(), r.direction(), r.get_depth());
            return ray_color(continued_ray, depth, world, lights);
        }

        ray scattered;
        color attenuation;
        double pdf_value;
        color color_from_emission = rec.mat->emitted(r, rec, rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered, pdf_value))
            return color_from_emission;

        hittable_pdf light_pdf(lights, rec.p);
        auto p0 = make_shared<hittable_pdf>(lights, rec.p);
        auto p1 = make_shared<cosine_pdf>(rec.normal);
        mixture_pdf mixed_pdf(p0, p1);

        scattered = ray(rec.p, mixed_pdf.generate(), r.get_depth());
        pdf_value = mixed_pdf.value(scattered.direction());

        double scattering_pdf = rec.mat->scattering_pdf(r, rec, scattered);

        color sample_color = ray_color(scattered, depth-1, world, lights);
        color color_from_scatter = (attenuation * scattering_pdf * sample_color) / pdf_value;

        return color_from_emission + color_from_scatter;
    }
};

#endif