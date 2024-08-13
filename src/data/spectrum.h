#include <vector>
#include <cmath>
#include <algorithm>

class Spectrum {
private:
    static constexpr int NUM_SAMPLES = 60;
    static constexpr double LAMBDA_MIN = 380.0; // nm
    static constexpr double LAMBDA_MAX = 730.0; // nm
    static constexpr double LAMBDA_RANGE = LAMBDA_MAX - LAMBDA_MIN;

    std::vector<double> samples;

public:
    Spectrum() : samples(NUM_SAMPLES, 0.0) {}

    explicit Spectrum(double value) : samples(NUM_SAMPLES, value) {}

    // Initialize spectrum with a vector of samples
    explicit Spectrum(const std::vector<double>& _samples) : samples(_samples) {
        if (samples.size() != NUM_SAMPLES) {
            throw std::invalid_argument("Incorrect number of samples");
        }
    }

    // Get the value at a specific wavelength
    double getValue(double wavelength) const {
        int index = static_cast<int>((wavelength - LAMBDA_MIN) / LAMBDA_RANGE * NUM_SAMPLES);
        index = std::clamp(index, 0, NUM_SAMPLES - 1);
        return samples[index];
    }

    // Set the value at a specific wavelength
    void setValue(double wavelength, double value) {
        int index = static_cast<int>((wavelength - LAMBDA_MIN) / LAMBDA_RANGE * NUM_SAMPLES);
        if (index >= 0 && index < NUM_SAMPLES) {
            samples[index] = value;
        }
    }

    // Basic arithmetic operations
    Spectrum operator+(const Spectrum& other) const {
        Spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.samples[i] = samples[i] + other.samples[i];
        }
        return result;
    }

    Spectrum operator*(const Spectrum& other) const {
        Spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.samples[i] = samples[i] * other.samples[i];
        }
        return result;
    }

    Spectrum operator*(double scalar) const {
        Spectrum result;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            result.samples[i] = samples[i] * scalar;
        }
        return result;
    }

    // Convert spectrum to RGB (simplified method)
    std::tuple<double, double, double> toRGB() const {
        // This is a very simplified conversion and not physically accurate
        // A proper implementation would use color matching functions
        double r = 0, g = 0, b = 0;
        for (int i = 0; i < NUM_SAMPLES; ++i) {
            double lambda = LAMBDA_MIN + (LAMBDA_RANGE * i) / NUM_SAMPLES;
            if (lambda < 490) r += samples[i];
            else if (lambda < 580) g += samples[i];
            else b += samples[i];
        }
        double scale = 1.0 / NUM_SAMPLES;
        return {r * scale, g * scale, b * scale};
    }

    // Static method to create a spectrum from a wavelength
    static Spectrum fromWavelength(double wavelength) {
        Spectrum result;
        result.setValue(wavelength, 1.0);
        return result;
    }
};