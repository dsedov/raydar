#ifndef RENDER_H
#define RENDER_H

#include "raydar.h"
#include "data/interval.h"
#include "data/hittable.h"
#include "image/image.h"
#include "core/material.h"
#include "core/camera.h"
#include <thread>

#include <QObject>
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

class render : public QObject{
    Q_OBJECT
public:

    int samples_per_pixel = 64;
    int max_depth         = 10;
    rd::core::camera camera;

    render(Image & image_buffer, rd::core::camera camera);
    int mtpool_bucket_prog_render(const hittable& world, const hittable& lights, std::atomic<bool>& should_continue_rendering);

signals:
    void progressUpdated(int current, int total);

private:
    /* Private Camera Variables Here */
    Image & image_buffer;
    vec3 pixel00_loc;
    vec3 pixel_delta_u;
    vec3 pixel_delta_v;
    int    sqrt_spp; 
    double recip_sqrt_spp;       
    vec3 u, v, w;              
    color background_color = color(0.0, 0.0, 0.0) ;
    double pixel_samples_scale;

    void initialize(bool is_vertical_fov = false, bool fov_in_degrees = true);
    ray get_ray(int i, int j, int s_i, int s_j, int depth) const ;
    vec3 sample_square_stratified(int s_i, int s_j) const ;
    void process_bucket(const Bucket& bucket, const hittable& world, const hittable& lights) ;
    spectrum ray_color(const ray& r, int depth, const hittable& world, const hittable& lights) const ;
    
    void updateProgress(int current, int total);
};

#endif // RENDER_H