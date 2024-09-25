#include <cmath>
#include <vector>
#include <algorithm>

class SPDGenerator {
private:
    static constexpr int RESPONSE_SAMPLES = 31;
    static constexpr float START_WAVELENGTH = 400.0f;
    static constexpr float END_WAVELENGTH = 700.0f;
    static constexpr float WAVELENGTH_STEP = (END_WAVELENGTH - START_WAVELENGTH) / (RESPONSE_SAMPLES - 1);

    // CIE daylight model coefficients
    static constexpr double S0[] = {0.04, 6.0, 29.6, 55.3, 57.3, 61.8, 61.5, 68.8, 63.4, 65.8, 94.8, 104.8, 105.9, 96.8, 113.9, 125.6, 125.5, 121.3, 121.3, 113.5, 113.1, 110.8, 106.5, 108.8, 105.3, 104.4, 100.0, 96.0, 95.1, 89.1, 90.5};
    static constexpr double S1[] = {0.02, 4.5, 22.4, 42.0, 40.6, 41.6, 38.0, 42.4, 38.5, 35.0, 43.4, 46.3, 43.9, 37.1, 36.7, 35.9, 32.6, 27.9, 24.3, 20.1, 16.2, 13.2, 8.6, 6.1, 4.2, 1.9, 0.0, -1.6, -3.5, -3.5, -5.8};
    static constexpr double S2[] = {0.0, 2.0, 4.0, 8.5, 7.8, 6.7, 5.3, 6.1, 3.0, 1.2, -1.1, -0.5, -0.7, -1.2, -2.6, -2.9, -2.8, -2.6, -2.6, -1.8, -1.5, -1.3, -1.2, -1.0, -0.5, -0.3, 0.0, 0.2, 0.5, 2.1, 3.2};

public:
    static std::vector<double> generateSPD(double temperature) {
        std::vector<double> spd(RESPONSE_SAMPLES);

        if (temperature < 4000.0) {
            // Use Planck's law for temperatures below 4000K
            for (int i = 0; i < RESPONSE_SAMPLES; ++i) {
                double wavelength = START_WAVELENGTH + i * WAVELENGTH_STEP;
                spd[i] = blackBodySpectrum(wavelength * 1e-9, temperature);
            }
        } else {
            // Use CIE method for daylight SPD
            double xD = calculateChromaticityX(temperature);
            double yD = calculateChromaticityY(xD);
            double M1 = (-1.3515 - 1.7703 * xD + 5.9114 * yD) / (0.0241 + 0.2562 * xD - 0.7341 * yD);
            double M2 = (0.0300 - 31.4424 * xD + 30.0717 * yD) / (0.0241 + 0.2562 * xD - 0.7341 * yD);

            for (int i = 0; i < RESPONSE_SAMPLES; ++i) {
                spd[i] = S0[i] + M1 * S1[i] + M2 * S2[i];
            }
        }

        // Normalize SPD
        double max_value = *std::max_element(spd.begin(), spd.end());
        for (double& value : spd) {
            value /= max_value;
        }

        return spd;
    }

private:
    static double blackBodySpectrum(double wavelength, double temperature) {
        const double h = 6.62607015e-34; // Planck constant
        const double c = 299792458.0;    // Speed of light
        const double k = 1.380649e-23;   // Boltzmann constant

        double numerator = 2.0 * h * c * c / std::pow(wavelength, 5);
        double denominator = std::exp((h * c) / (wavelength * k * temperature)) - 1.0;
        return numerator / denominator;
    }

    static double calculateChromaticityX(double temperature) {
        double x;
        if (temperature <= 7000.0) {
            x = -4.6070e9 / std::pow(temperature, 3) + 2.9678e6 / std::pow(temperature, 2) + 0.09911e3 / temperature + 0.244063;
        } else {
            x = -2.0064e9 / std::pow(temperature, 3) + 1.9018e6 / std::pow(temperature, 2) + 0.24748e3 / temperature + 0.237040;
        }
        return x;
    }

    static double calculateChromaticityY(double x) {
        return -3.000 * x * x + 2.870 * x - 0.275;
    }
};