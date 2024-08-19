class BlueNoiseDithering {
private:
    std::vector<std::vector<vec3>> blue_noise_mask;
    int mask_size;

    // Gaussian filter for void and cluster method
    float gaussian(float x, float y, float sigma) {
        return std::exp(-(x*x + y*y) / (2 * sigma * sigma));
    }

    // Compute the energy at a given point
    float compute_energy(const std::vector<std::vector<float>>& binary, int x, int y) {
        float energy = 0.0f;
        float sigma = 1.5f;
        for (int dy = -5; dy <= 5; ++dy) {
            for (int dx = -5; dx <= 5; ++dx) {
                int nx = (x + dx + mask_size) % mask_size;
                int ny = (y + dy + mask_size) % mask_size;
                energy += binary[ny][nx] * gaussian(dx, dy, sigma);
            }
        }
        return energy;
    }

    // Generate blue noise mask using void and cluster method
    void generate_blue_noise_mask() {
        std::vector<std::vector<float>> binary(mask_size, std::vector<float>(mask_size, 0.0f));
        std::vector<std::pair<int, int>> active_points;

        // Initial random points
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, mask_size - 1);
        for (int i = 0; i < mask_size * mask_size / 10; ++i) {
            int x = dis(gen), y = dis(gen);
            binary[y][x] = 1.0f;
            active_points.emplace_back(x, y);
        }

        // Void and cluster
        while (active_points.size() < mask_size * mask_size) {
            // Find the largest void
            float max_void_energy = -1.0f;
            int void_x = 0, void_y = 0;
            for (int y = 0; y < mask_size; ++y) {
                for (int x = 0; x < mask_size; ++x) {
                    if (binary[y][x] == 0.0f) {
                        float energy = compute_energy(binary, x, y);
                        if (energy > max_void_energy) {
                            max_void_energy = energy;
                            void_x = x;
                            void_y = y;
                        }
                    }
                }
            }

            // Fill the void
            binary[void_y][void_x] = 1.0f;
            active_points.emplace_back(void_x, void_y);
        }

        // Convert binary pattern to dither values
        for (int y = 0; y < mask_size; ++y) {
            for (int x = 0; x < mask_size; ++x) {
                float rank = std::find(active_points.begin(), active_points.end(), std::make_pair(x, y)) - active_points.begin();
                blue_noise_mask[y][x] = vec3(rank / (mask_size * mask_size), rank / (mask_size * mask_size), 0);
            }
        }
    }

public:
    BlueNoiseDithering(int size) : mask_size(size) {
        blue_noise_mask.resize(mask_size, std::vector<vec3>(mask_size));
        generate_blue_noise_mask();
    }

    vec3 sample_square_dithered(int s_i, int s_j, int pixel_x, int pixel_y, int sqrt_spp) const {
        // Get the blue noise offset for this pixel
        int mask_x = pixel_x % mask_size;
        int mask_y = pixel_y % mask_size;
        vec3 blue_noise_offset = blue_noise_mask[mask_y][mask_x];

        // Calculate the stratified sample position
        float recip_sqrt_spp = 1.0f / sqrt_spp;
        float px = ((s_i + blue_noise_offset.x()) * recip_sqrt_spp) - 0.5f;
        float py = ((s_j + blue_noise_offset.y()) * recip_sqrt_spp) - 0.5f;

        return vec3(px, py, 0);
    }
};

// Usage example
BlueNoiseDithering dithering(128); 