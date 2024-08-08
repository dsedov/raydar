#ifndef CAMERA_H
#define CAMERA_H

#include "raydar.h"
#include "data/interval.h"
#include "data/hittable.h"
#include "image/image.h"
#include <thread>
class camera {
  public:

    int samples_per_pixel = 64;
    int max_depth         = 10;
    /* Public Camera Parameters Here */
    camera(Image & image_buffer) : image_buffer(image_buffer) {
       
    }

    void render(const hittable& world) {
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
    }
    void mt_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();

        std::vector<std::thread> threads;
        std::atomic<int> scanlines_remaining(image_buffer.height());
        std::mutex cout_mutex;

        auto render_scanline = [&](int j) {
            for (int i = 0; i < image_buffer.width(); i++) {
                color pixel_color(0,0,0);
                for (int sample = 0; sample < samples_per_pixel; sample++) {
                    ray r = get_ray(i, j);
                    pixel_color += ray_color(r, max_depth, world);
                }
                pixel_color *= pixel_samples_scale;
                image_buffer.set_pixel(i, j, pixel_color);
            }

        };

        for (int j = 0; j < image_buffer.height(); j++) {
            threads.emplace_back(render_scanline, j);
        }

        for (auto& thread : threads) {
            thread.join();
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "\nRendering time: " << duration.count() << " seconds" << std::endl;
    }
  private:
    /* Private Camera Variables Here */
    Image & image_buffer;
    vec3 camera_center;
    vec3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    
    double pixel_samples_scale;

    void initialize() {

        auto focal_length = 1.0;
        auto viewport_height = 2.0;
        auto viewport_width = viewport_height * (double(image_buffer.width())/image_buffer.height());
        pixel_samples_scale = 1.0 / samples_per_pixel;
        camera_center = point3(0, 0, 0);

        // Calculate the vectors across the horizontal and down the vertical viewport edges.
        auto viewport_u = vec3(viewport_width, 0, 0);
        auto viewport_v = vec3(0, -viewport_height, 0);

        // Calculate the horizontal and vertical delta vectors from pixel to pixel.
        pixel_delta_u = viewport_u / image_buffer.width();
        pixel_delta_v = viewport_v / image_buffer.height();

        // Calculate the location of the upper left pixel.
        auto viewport_upper_left = camera_center - vec3(0, 0, focal_length) - viewport_u/2 - viewport_v/2;
        pixel00_loc = viewport_upper_left + 0.5 * (pixel_delta_u + pixel_delta_v);
    }
    ray get_ray(int i, int j) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.

        auto offset = sample_square();
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = camera_center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction);
    }
    vec3 sample_square() const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }
    color ray_color(const ray& r, int depth, const hittable& world) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        if (world.hit(r, interval(0.001, infinity), rec)) { 
            vec3 direction = rec.normal + random_unit_vector();
            //vec3 direction = random_on_hemisphere(rec.normal);
            return 0.5 * ray_color(ray(rec.p, direction), depth-1, world);
            //return 0.5 * (rec.normal + color(1,1,1));
        }

        vec3 unit_direction = unit_vector(r.direction());
        auto a = 0.5*(unit_direction.y() + 1.0);
        return (1.0-a)*color(1.0, 1.0, 1.0) + a*color(0.5, 0.7, 1.0);
    }
};

#endif