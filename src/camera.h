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
#include <queue>

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

class MitchellNetravaliFilter {
private:
    double B, C;

    double p(double x) {
        x = std::abs(x);
        if (x < 1) {
            return ((12 - 9 * B - 6 * C) * x * x * x
                    + (-18 + 12 * B + 6 * C) * x * x
                    + (6 - 2 * B)) / 6;
        } else if (x < 2) {
            return ((-B - 6 * C) * x * x * x
                    + (6 * B + 30 * C) * x * x
                    + (-12 * B - 48 * C) * x
                    + (8 * B + 24 * C)) / 6;
        } else {
            return 0;
        }
    }

public:
    MitchellNetravaliFilter(double b = 1.0/3.0, double c = 1.0/3.0) : B(b), C(c) {}

    double filter(double x) {
        return p(x);
    }
};

struct Sample {
    float x, y;  // Sample position relative to pixel center
    color color; // Sample color
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
    
    int mt_render(const hittable& world) {
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
                        ray r = get_ray(i, j, s_i, s_j, 0);
                        pixel_color += ray_color(r, max_depth, world);
                    }
                }
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
        return duration.count();
    }
    int mtpool_prog_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();

        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(num_threads);
        std::queue<int> sample_queue;
        std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<int> samples_completed(0);
        std::atomic<bool> done(false);

        const int total_samples = sqrt_spp * sqrt_spp;
        ProgressBar progress_bar(total_samples);

        // Fill the queue with sample indices
        for (int s = 0; s < total_samples; ++s) {
            sample_queue.push(s);
        }

        auto worker = [&]() {
            while (true) {
                int sample_index;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (sample_queue.empty()) {
                        if (done) return;  // Exit if work is done
                        cv.wait(lock);  // Wait for more work or done signal
                        continue;  // Recheck condition after waking
                    }
                    sample_index = sample_queue.front();
                    sample_queue.pop();
                }

                int s_i = sample_index % sqrt_spp;
                int s_j = sample_index / sqrt_spp;

                // Process all pixels for this sample
                for (int j = 0; j < image_buffer.height(); ++j) {
                    for (int i = 0; i < image_buffer.width(); ++i) {
                        ray r = get_ray(i, j, s_i, s_j, 0);
                        color pixel_color = ray_color(r, max_depth, world);
                        image_buffer.add_to_pixel(i, j, pixel_color * pixel_samples_scale);
                    }
                }

                int completed = samples_completed.fetch_add(1) + 1;
                progress_bar.update(completed);

                if (completed == total_samples) {
                    done = true;
                    cv.notify_all();  // Wake up all threads to check done condition
                }
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

        std::cout << std::endl; // Move to the next line after progress bar

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "Rendering time: " << duration.count() << " seconds" << std::endl;
        return duration.count();
    }
    /*
    int mtpool_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();

        const int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads(num_threads);
        std::queue<int> scanline_queue;
        std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<int> scanlines_completed(0);
        std::atomic<bool> done(false);

        ProgressBar progress_bar(image_buffer.height());

        // Fill the queue with scanlines
        for (int j = 0; j < image_buffer.height(); ++j) {
            scanline_queue.push(j);
        }

        auto worker = [&]() {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> dis(0.0, 1.0);
            while (true) {
                int j;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (scanline_queue.empty()) {
                        if (done) return;  // Exit if work is done
                        cv.wait(lock);  // Wait for more work or done signal
                        continue;  // Recheck condition after waking
                    }
                    j = scanline_queue.front();
                    scanline_queue.pop();
                }

                // Process scanline
                for (int i = 0; i < image_buffer.width(); i++) {
                    
                    color pixel_color(0,0,0);
                    for (int s_j = 0; s_j < sqrt_spp; s_j++) {
                        for (int s_i = 0; s_i < sqrt_spp; s_i++) {

                            ray r = get_ray(i, j, s_i, s_j, 0);
                            pixel_color += ray_color(r, max_depth, world);
                        }
                    }
                    pixel_color *= pixel_samples_scale;
                    image_buffer.set_pixel(i, j, pixel_color);
                }

                int completed = scanlines_completed.fetch_add(1) + 1;
                progress_bar.update(completed);

                if (completed == image_buffer.height()) {
                    done = true;
                    cv.notify_all();  // Wake up all threads to check done condition
                }
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

        std::cout << std::endl; // Move to the next line after progress bar

        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(end_time - start_time);
        std::cout << "Rendering time: " << duration.count() << " seconds" << std::endl;
        return duration.count();
    }
  
    int adaptive_mt_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        std::atomic<int> scanlines_completed(0);
        ProgressBar progress_bar(image_buffer.height());

        const int min_samples = 4; // Minimum number of samples per pixel
        const int max_samples = sqrt_spp * sqrt_spp; // Maximum number of samples per pixel
        const float convergence_threshold = 0.001f; // Threshold for color change

        auto render_scanline = [&](int j) {
            for (int i = 0; i < image_buffer.width(); i++) {
                color pixel_color(0,0,0);
                color prev_pixel_color(0,0,0);
                int samples = 0;
                bool converged = false;
                std::srand(j*i + j + i + std::time(nullptr));
                while (samples < max_samples && !converged) {
                    color sample_color(0,0,0);
                    int batch_size = std::min(8, max_samples - samples); // Sample in batches of 4

                    for (int s = 0; s < batch_size; s++) {
                        int s_i = samples % sqrt_spp;
                        int s_j = samples / sqrt_spp;
                        ray r = get_ray(i, j, s_i, s_j, 0);
                        sample_color += ray_color(r, max_depth, world);
                        samples++;
                    }

                    pixel_color += sample_color;
                    color current_average = pixel_color / samples;

                    if (samples >= min_samples) {
                        float color_diff = (current_average - prev_pixel_color).length();
                        if (color_diff < convergence_threshold) {
                            converged = true;
                        }
                    }

                    prev_pixel_color = current_average;
                }

                pixel_color /= samples; // Normalize by actual number of samples
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
        return duration.count();
    }
    
    int adaptive_filtered_mt_render(const hittable& world) {
        initialize();
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<std::thread> threads;
        std::atomic<int> scanlines_completed(0);
        ProgressBar progress_bar(image_buffer.height());
        const int min_samples = 4;
        const int max_samples = sqrt_spp * sqrt_spp;
        const float convergence_threshold = 0.001f;
        const float filter_width = 1.0f; // Adjust as needed
        MitchellNetravaliFilter filter;

        auto render_scanline = [&](int j) {
            for (int i = 0; i < image_buffer.width(); i++) {
                std::vector<Sample> samples;
                color pixel_color(0,0,0);
                color prev_pixel_color(0,0,0);
                int sample_count = 0;
                bool converged = false;
                std::srand(j*i + j + i + std::time(nullptr));
                while (sample_count < max_samples && !converged) {
                    int batch_size = std::min(8, max_samples - sample_count);
                    for (int s = 0; s < batch_size; s++) {
                        int s_i = sample_count % sqrt_spp;
                        int s_j = sample_count / sqrt_spp;
                        ray r = get_ray(i, j, s_i, s_j, 0);
                        color sample_color = ray_color(r, max_depth, world);
                        
                        // Calculate sample position relative to pixel center
                        float x_offset = (float)s_i / sqrt_spp - 0.5f;
                        float y_offset = (float)s_j / sqrt_spp - 0.5f;
                        
                        samples.push_back({x_offset, y_offset, sample_color});
                        sample_count++;
                    }

                    // Apply Mitchell-Netravali filter to reconstruct pixel color
                    float total_weight = 0.0f;
                    color filtered_color(0,0,0);
                    for (const auto& sample : samples) {
                        float distance = std::sqrt(sample.x*sample.x + sample.y*sample.y);
                        float weight = filter.filter(distance / filter_width);
                        filtered_color += sample.color * weight;
                        total_weight += weight;
                    }
                    if (total_weight > 0) {
                        filtered_color /= total_weight;
                    }

                    pixel_color = filtered_color;

                    if (sample_count >= min_samples) {
                        float color_diff = (pixel_color - prev_pixel_color).length();
                        if (color_diff < convergence_threshold) {
                            converged = true;
                        }
                    }
                    prev_pixel_color = pixel_color;
                }

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
        return duration.count();
    }
    */
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
    ray get_ray(int i, int j, int s_i, int s_j, int depth) const {
        // Construct a camera ray originating from the origin and directed at randomly sampled
        // point around the pixel location i, j.


        auto offset = sample_square_stratified_van_der_corput(s_i, s_j);
        auto pixel_sample = pixel00_loc
                          + ((i + offset.x()) * pixel_delta_u)
                          + ((j + offset.y()) * pixel_delta_v);

        auto ray_origin = camera_center;
        auto ray_direction = pixel_sample - ray_origin;

        return ray(ray_origin, ray_direction, depth);
    }
    vec3 sample_square_stratified(int s_i, int s_j) const {

        auto px = ((s_i + random_double()) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + random_double()) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }

    vec3 sample_square_stratified_van_der_corput(int s_i, int s_j) const {
        auto van_der_corput = [](unsigned int n, unsigned int base) {
            double q = 0, b = 1.0 / base;
            while (n > 0) {
                q += (n % base) * b;
                n /= base;
                b /= base;
            }
            return q;
        };

        unsigned int index = s_i * sqrt_spp + s_j;
        double ldx = van_der_corput(index, 2);  // Base 2 for x
        double ldy = van_der_corput(index, 3);  // Base 3 for y

        // Map low-discrepancy sequence to the sub-pixel
        auto px = ((s_i + ldx) * recip_sqrt_spp) - 0.5;
        auto py = ((s_j + ldy) * recip_sqrt_spp) - 0.5;

        return vec3(px, py, 0);
    }
    vec3 sample_square(int s_i, int s_j) const {
        // Returns the vector to a random point in the [-.5,-.5]-[+.5,+.5] unit square.
        std::srand(s_i * sqrt_spp + s_j); // Set the seed based on s_i and s_j
        return vec3(random_double() - 0.5, random_double() - 0.5, 0);
    }
    color ray_color(ray& r, int depth, const hittable& world) const {
        
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
            ray continued_ray(rec.p + bias * r.direction(), r.direction(), r.get_depth());
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