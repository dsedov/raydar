import numpy as np
from scipy import interpolate


# Data from 380nm to 780nm at 5nm intervals
standard_wavelengths = np.arange(380, 785, 5)
# Standard CIE 1931 2-degree color matching function for x (red)
standard_x = np.array([
    0.000134, 0.000232, 0.000435, 0.000776, 0.001616, 0.002777, 0.005028, 0.009070, 0.014820, 0.023190,
    0.033620, 0.045370, 0.056510, 0.064240, 0.068110, 0.068590, 0.067230, 0.064970, 0.062280, 0.059570,
    0.057250, 0.055230, 0.053290, 0.051260, 0.049100, 0.046770, 0.044410, 0.042080, 0.039810, 0.037650,
    0.035550, 0.033560, 0.031700, 0.029960, 0.028340, 0.026830, 0.025420, 0.024100, 0.022860, 0.021690,
    0.020580, 0.019520, 0.018500, 0.017510, 0.016550, 0.015620, 0.014720, 0.013840, 0.013000, 0.012180,
    0.011390, 0.010630, 0.009900, 0.009200, 0.008530, 0.007890, 0.007290, 0.006730, 0.006210, 0.005730,
    0.005290, 0.004890, 0.004520, 0.004180, 0.003870, 0.003580, 0.003320, 0.003080, 0.002850, 0.002640,
    0.002450, 0.002270, 0.002100, 0.001940, 0.001800, 0.001660, 0.001540, 0.001420, 0.001310, 0.001210,
    0.001120
])
# Standard CIE 1931 2-degree color matching function for x (red)
standard_y = np.array([
    0.000039, 0.000064, 0.000120, 0.000217, 0.000396, 0.000640, 0.001210, 0.002180, 0.004000, 0.007300,
    0.011600, 0.016840, 0.023000, 0.029800, 0.038000, 0.048000, 0.060000, 0.073900, 0.090980, 0.112600,
    0.139020, 0.169300, 0.208020, 0.258600, 0.323000, 0.407300, 0.503000, 0.608200, 0.710000, 0.793200,
    0.862000, 0.914850, 0.954000, 0.980300, 0.994950, 1.000000, 0.995000, 0.978600, 0.952000, 0.915400,
    0.870000, 0.816300, 0.757000, 0.694900, 0.631000, 0.566800, 0.503000, 0.441200, 0.381000, 0.321000,
    0.265000, 0.217000, 0.175000, 0.138200, 0.107000, 0.081600, 0.061000, 0.044580, 0.032000, 0.023200,
    0.017000, 0.011920, 0.008210, 0.005723, 0.004102, 0.002929, 0.002091, 0.001484, 0.001047, 0.000740,
    0.000520, 0.000361, 0.000249, 0.000172, 0.000120, 0.000085, 0.000060, 0.000042, 0.000030, 0.000021,
    0.000015
])
# Standard CIE 1931 2-degree color matching function for z (blue)
standard_z = np.array([
    0.006450, 0.010550, 0.020050, 0.036210, 0.067850, 0.110200, 0.207400, 0.371300, 0.645600, 1.039050,
    1.385600, 1.622960, 1.747060, 1.782600, 1.772110, 1.744100, 1.669200, 1.528100, 1.287640, 1.041900,
    0.812950, 0.616200, 0.465180, 0.353300, 0.272000, 0.212300, 0.158200, 0.111700, 0.078250, 0.057250,
    0.042160, 0.029840, 0.020300, 0.013400, 0.008750, 0.005750, 0.003900, 0.002750, 0.002100, 0.001800,
    0.001650, 0.001400, 0.001100, 0.001000, 0.000800, 0.000600, 0.000340, 0.000240, 0.000190, 0.000100,
    0.000050, 0.000030, 0.000020, 0.000010, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000, 0.000000,
    0.000000
])
# Verify that the lengths match for all color components
assert len(standard_wavelengths) == len(standard_x) == len(standard_y) == len(standard_z), "Mismatch in data lengths"

# Create interpolation functions for each color component
f_x = interpolate.interp1d(standard_wavelengths, standard_x, kind='cubic', fill_value='extrapolate')
f_y = interpolate.interp1d(standard_wavelengths, standard_y, kind='cubic', fill_value='extrapolate')
f_z = interpolate.interp1d(standard_wavelengths, standard_z, kind='cubic', fill_value='extrapolate')

# Generate new wavelengths from 300nm to 800nm at 1nm intervals
new_wavelengths = np.arange(300, 801)

# Interpolate and extrapolate the data for each color component
new_x = np.maximum(f_x(new_wavelengths), 0)  # Set negative values to zero
new_y = np.maximum(f_y(new_wavelengths), 0)
new_z = np.maximum(f_z(new_wavelengths), 0)

# Print the results for each color component

print("const std::array<float, RESPONSE_SAMPLES> Spectrum::r_response_curve = {")
for value in new_x:
    print(f"    {value:.8f}f,", end="")
print("};")

print("const std::array<float, RESPONSE_SAMPLES> Spectrum::g_response_curve = {")
for value in new_y:
    print(f"    {value:.8f}f,", end="")
print("};")

print("const std::array<float, RESPONSE_SAMPLES> Spectrum::b_response_curve = {")
for value in new_z:
    print(f"    {value:.8f}f,", end="")
print("};")


