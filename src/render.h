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
const float CACHE_PROBABILITY = 0.7f;

struct Bucket {
    int start_x, start_y, end_x, end_y;
};
struct Sample {
    float x, y;  // Sample position relative to pixel center
    color color; // Sample color
};
struct CacheEntry {
    hit_record rec;
    bool hit;
};

// Custom hash function for std::pair<int, int>
struct PairHash {
    std::size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

using CacheType = std::unordered_map<std::pair<int, int>, CacheEntry, PairHash>;

class Cache {
private:
    CacheType cache;
    std::mt19937 rng;
    std::uniform_real_distribution<float> dist;

public:
    Cache() : rng(std::random_device{}()), dist(0.0f, 1.0f) {}

    bool get(int x, int y, CacheEntry& entry) {
        auto it = cache.find({x, y});
        if (it != cache.end() && dist(rng) < CACHE_PROBABILITY) {
            entry = it->second;
            return true;
        }
        return false;
    }

    void set(int x, int y, const CacheEntry& entry) {
        cache[{x, y}] = entry;
    }

    void clear() {
        cache.clear();
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
        std::queue<Bucket> bucket_queue;
        std::mutex queue_mutex;
        std::condition_variable cv;
        std::atomic<int> buckets_completed(0);
        std::atomic<bool> done(false);
        const int total_samples = sqrt_spp * sqrt_spp;
        ProgressBar progress_bar(total_samples);

        // Create buckets
        for (int y = 0; y < image_buffer.height(); y += BUCKET_SIZE) {
            for (int x = 0; x < image_buffer.width(); x += BUCKET_SIZE) {
                Bucket b{x, y, 
                        std::min(x + BUCKET_SIZE, image_buffer.width()), 
                        std::min(y + BUCKET_SIZE, image_buffer.height())};
                bucket_queue.push(b);
            }
        }
        const int total_buckets = bucket_queue.size();

        auto worker = [&]() {
            Cache local_cache;
            while (true) {
                Bucket bucket;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    if (bucket_queue.empty()) {
                        if (done) return;
                        cv.wait(lock);
                        continue;
                    }
                    bucket = bucket_queue.front();
                    bucket_queue.pop();
                }

                // Process the bucket
                for (int j = bucket.start_y; j < bucket.end_y; ++j) {
                    for (int i = bucket.start_x; i < bucket.end_x; ++i) {
                        color pixel_color(0, 0, 0);
                        for (int s = 0; s < total_samples; ++s) {
                            int s_i = s % sqrt_spp;
                            int s_j = s / sqrt_spp;
                            ray r = get_ray(i, j, s_i, s_j, 0);
                            pixel_color += ray_color(r, max_depth, world, local_cache, i, j);
                        }
                        image_buffer.set_pixel(i, j, pixel_color * pixel_samples_scale);
                    }
                }

                local_cache.clear();  // Clear cache after processing each bucket

                int completed = buckets_completed.fetch_add(1) + 1;
                if (completed == total_buckets) {
                    done = true;
                    cv.notify_all();
                }
            }
        };

        // Start worker threads
        for (int t = 0; t < num_threads; ++t) {
            threads[t] = std::thread(worker);
        }

        // Main thread handles progress updates
        while (!done) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            int current_progress = buckets_completed.load() * total_samples / total_buckets;
            progress_bar.update(current_progress);
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
    color ray_color(ray& r, int depth, const hittable& world, Cache& cache, int x = -1, int y = -1) const {
        if (depth <= 0)
            return color(0,0,0);

        hit_record rec;
        bool is_primary = depth == max_depth;
        bool hit;

        if (is_primary && x != -1 && y != -1) {
            CacheEntry cache_entry;
            if (cache.get(x, y, cache_entry)) {
                hit = cache_entry.hit;
                rec = cache_entry.rec;
            } else {
                hit = world.hit(r, interval(0.001, infinity), rec);
                cache.set(x, y, {rec, hit});
            }
        } else {
            hit = world.hit(r, interval(0.001, infinity), rec);
        }

        if (!hit) {
            return background_color;
        }

        // For primary rays where depth is equal to max_depth
        if (depth == max_depth && !rec.mat->is_visible()) {
            // Continue ray in the same direction with a small bias
            const double bias = 0.0001;
            ray continued_ray(rec.p + bias * r.direction(), r.direction(), r.get_depth());
            return ray_color(continued_ray, depth, world, cache);
        }

        ray scattered;
        color attenuation;
        color emitted = rec.mat->emitted(rec.u, rec.v, rec.p);
        if (!rec.mat->scatter(r, rec, attenuation, scattered)){
            return emitted;
        }
        return emitted + attenuation * ray_color(scattered, depth-1, world, cache);
    }
};

#endif