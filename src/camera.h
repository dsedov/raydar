#ifndef CAMERA_H
#define CAMERA_H

#include "raydar.h"
#include "data/interval.h"
#include "data/hittable.h"
#include "image/image.h"
#include "material.h"
#include <thread>

#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <vector>
#include <random>
#include <algorithm>

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

class camera {
  public:

    int samples_per_pixel = 64;
    int max_depth         = 10;
    double fov;
    point3 lookfrom;   // Point camera is looking from
    point3 lookat;  // Point camera is looking at
    vec3   vup;
    /* Public Camera Parameters Here */
    camera(Image & image_buffer) : image_buffer(image_buffer) {
       
    }
    float color_variance(const color& c1, const color& c2) {
    return (c1 - c2).length_squared() / 3.0f;
}
    /*void render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();

        for (int j = 0; j < image_buffer.height(); j++) {
            std::clog << "\rScanlines remaining: " << (image_buffer.height() - j) << ' ' << std::flush;
            for (int i = 0; i < image_buffer.width(); i++) {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                pixel_color *= pixel_samples_scale;
                image_buffer.set_pixel(i, j, pixel_color);
            }
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "\nRendering time: " << duration.count() << " seconds" << std::endl;
    }*/
    void mt_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        std::atomic<int> scanlines_completed(0);
        ProgressBar progress_bar(image_buffer.height());

        auto render_scanline = [&](int j) {
            for (int i = 0; i < image_buffer.width(); i++) {
                color pixel_color(0,0,0);
                for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                    for (int s_i = 0; s_i < sqrt_spp; s_i++) {
                        ray r = get_ray(i, j, s_i, s_j);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                }
                /*
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }*/

                pixel_color *= pixel_samples_scale;
                image_buffer.set_pixel(i, j, pixel_color);
            }
            
            int completed = scanlines_completed.fetch_add(1) + 1;
            progress_bar.update(completed);
        };

        for (int j = 0; j < image_buffer.height(); j++) {
            threads.emplace_back(render_scanline, j);
        }

        for (auto& thread : threads) {
            thread.join();
        }

        std::cout << std::endl; // Move to the next line after progress bar

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "Rendering time: " << duration.count() << " seconds" << std::endl;
    }
    
    
  private:
    /* Private Camera Variables Here */
    Image & image_buffer;
    vec3 camera_center;
    vec3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    int    sqrt_spp;             // Square root of number of samples per pixel
    double recip_sqrt_spp;       // 1 / sqrt_spp
    vec3 u, v, w;              // Camera frame basis vectors
    color background_color = color(0.0, 0.0, 0.0) ;
    double pixel_samples_scale;

    void initialize(bool is_vertical_fov = false, bool fov_in_degrees = true) {
        auto focal_length = (lookfrom - lookat).length();
        
        // Convert FOV to radians if it's in degrees
        double fov_radians = fov_in_degrees ? degrees_to_radians(fov) : fov;
        
        // Calculate viewport dimensions based on FOV type
        double viewport_height, viewport_width;
        if (is_vertical_fov) {
            viewport_height = 2.0 * focal_length * tan(fov_radians / 2.0);
            viewport_width = viewport_height * (double(image_buffer.width()) / image_buffer.height());
        } else {
            viewport_width = 2.0 * focal_length * tan(fov_radians / 2.0);
            viewport_height = viewport_width / (double(image_buffer.width()) / image_buffer.height());
        }

        //pixel_samples_scale = 1.0 / samples_per_pixel;

        sqrt_spp = int(std::sqrt(samples_per_pixel));
        pixel_samples_scale = 1.0 / (sqrt_spp * sqrt_spp);
        recip_sqrt_spp = 1.0 / sqrt_spp;

        camera_center = lookfrom;

        // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
        w = unit_vector(lookfrom - lookat);
        u = unit_vector(cross(vup, w));
        v = cross(w, u);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        vec3 viewport_u = viewport_width * u;    // Vector across viewport horizontal edge
        vec3 viewport_v = viewport_height * -v;  // Vector down viewport vertical edge

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_buffer.width();
        pixel_delta_v = viewport_v / image_buffer.height();

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = camera_center - (focal_length * w) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }
    ray get_ray(int i, int j, int s_i, int s_j) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        //auto offset = sample_square();
        auto offset = sample_square_stratified(s_i, s_j);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = camera_center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }
    vec3 sample_square_stratified(int s_i, int s_j) const {
        // Returns the vector to a random point in the square sub-pixel specified by grid
        // indices s_i and s_j, for an idealized unit square pixel [-.5,-.5] to [+.5,+.5].

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }
    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }
    color ray_color(const ray& r, int depth, const hittable& world) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        if (!world.hit(r, interval(0.001, infinity), rec)) { 
            return background_color;
        }

        // For primary rays where depth is equal to max_depth
        if (depth == max_depth && !rec.mat->is_visible()) {
            // Continue ray in the same direction with a small bias
            const double bias = 0.0001;
            ray continued_ray(rec.p + bias * r.direction(), r.direction());
            return ray_color(continued_ray, depth, world);
        }

        ray scattered;
        color attenuation;
        
        color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);

        if (!rec.mat->scatter(r, rec, attenuation, scattered)){
            return emitted;
        }
        return emitted + attenuation * ray_color(scattered, depth-1, world);
    }
};

#endif