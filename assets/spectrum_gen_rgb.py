import numpy as np
import matplotlib.pyplot as plt
from colormath.color_objects import sRGBColor, LabColor
from colormath.color_conversions import convert_color
from colormath import color_diff
from tqdm import tqdm

# Patch numpy to handle the deprecated asscalar function
def numpy_asscalar(a):
    return a.item() if hasattr(a, 'item') else a

np.asscalar = numpy_asscalar

# Now we can safely import and use delta_e_cie2000
from colormath.color_diff import delta_e_cie2000

def rgb_to_hsv(rgb):
    rgb = rgb.reshape(-1, 1, 3)
    r, g, b = rgb[:, :, 0], rgb[:, :, 1], rgb[:, :, 2]
    maxc = np.max(rgb, axis=2)
    minc = np.min(rgb, axis=2)
    v = maxc
    deltac = maxc - minc
    s = deltac / maxc
    deltac[deltac == 0] = 1  # to not divide by zero (those results in any way would be overridden in next lines)
    rc = (maxc - r) / deltac
    gc = (maxc - g) / deltac
    bc = (maxc - b) / deltac

    h = 4.0 + gc - rc
    h[g == maxc] = 2.0 + rc[g == maxc] - bc[g == maxc]
    h[r == maxc] = bc[r == maxc] - gc[r == maxc]
    h[minc == maxc] = 0.0

    h = (h / 6.0) % 1.0
    return np.concatenate([h, s, v], axis=1).squeeze()

def generate_rgb_swatches(num_swatches):
    # Generate a range of values with more emphasis on extremes and mid-range
    steps = max(3, int(np.cbrt(num_swatches)))
    values = np.concatenate([
        [0],  # Ensure 0 is included
        np.linspace(0.1, 0.9, steps - 2),  # Mid-range values
        [1]   # Ensure 1 is included
    ])
    
    # Generate RGB values
    rgb_values = np.array(np.meshgrid(values, values, values)).T.reshape(-1, 3)
    
    # Add pure red, green, blue
    rgb_values = np.vstack([rgb_values, [1, 0, 0], [0, 1, 0], [0, 0, 1]])
    
    # Add more grayscale values
    grayscale = np.linspace(0, 1, steps)[:, np.newaxis].repeat(3, axis=1)
    rgb_values = np.vstack([rgb_values, grayscale])

    earth_tones = np.array([
        [0.76, 0.60, 0.42],  # Tan
        [0.55, 0.27, 0.07],  # Brown
        [0.33, 0.42, 0.18],  # Olive
        [0.85, 0.65, 0.13],  # Golden
        [0.55, 0.47, 0.37],  # Taupe
        [0.73, 0.53, 0.40],  # Camel
        [0.80, 0.52, 0.25],  # Rust
        [0.45, 0.32, 0.28],  # Umber
        [0.60, 0.45, 0.33],  # Khaki
    ])
    rgb_values = np.vstack([rgb_values, earth_tones])
    
    # Add pastel colors
    pastel_colors = np.array([
        [0.93, 0.77, 0.80],  # Pastel Pink
        [0.80, 0.92, 0.77],  # Pastel Green
        [0.77, 0.83, 0.93],  # Pastel Blue
        [0.96, 0.87, 0.70],  # Pastel Yellow
        [0.85, 0.77, 0.93],  # Pastel Purple
        [0.96, 0.80, 0.69],  # Pastel Orange
    ])
    rgb_values = np.vstack([rgb_values, pastel_colors])
    
    # Remove duplicates
    rgb_values = np.unique(rgb_values, axis=0)
    
    # Convert RGB to Lab
    print("Converting RGB to Lab...")
    lab_values = np.array([convert_color(sRGBColor(r, g, b), LabColor).get_value_tuple() 
                           for r, g, b in tqdm(rgb_values, total=len(rgb_values))])
    
    # Select swatches with maximum color difference
    selected_indices = [0]  # Start with the first color
    print("Selecting swatches...")
    pbar = tqdm(total=min(num_swatches, len(rgb_values)) - 1)
    while len(selected_indices) < num_swatches and len(selected_indices) < len(rgb_values):
        last_selected = lab_values[selected_indices[-1]]
        color_diffs = np.array([delta_e_cie2000(LabColor(*last_selected), LabColor(*lab2)) 
                                for lab2 in lab_values])
        color_diffs[selected_indices] = -1  # Exclude already selected colors
        max_diff_index = np.argmax(color_diffs)
        selected_indices.append(max_diff_index)
        pbar.update(1)
    pbar.close()
    
    return rgb_values[selected_indices]

def create_swatch_image(rgb_values, swatch_size, page_width, page_height):
    swatches_per_row = page_width // swatch_size
    swatches_per_col = page_height // swatch_size
    total_swatches = swatches_per_row * swatches_per_col
    
    # Convert RGB to HSV for sorting
    hsv_values = rgb_to_hsv(rgb_values)
    
    # Sort by Hue, then Saturation, then Value
    sorted_indices = np.lexsort((hsv_values[:, 2], hsv_values[:, 1], hsv_values[:, 0]))
    sorted_rgb_values = rgb_values[sorted_indices]
    
    # Pad with white if we have fewer swatches than grid spaces
    if len(sorted_rgb_values) < total_swatches:
        padding = np.ones((total_swatches - len(sorted_rgb_values), 3))
        sorted_rgb_values = np.vstack([sorted_rgb_values, padding])
    
    image = sorted_rgb_values[:total_swatches].reshape(swatches_per_col, swatches_per_row, 3)
    image = image.repeat(swatch_size, axis=0).repeat(swatch_size, axis=1)
    
    return image

# Set parameters
num_swatches = 400  # Adjust as needed
swatch_size = 150   # 1 inch at 100 DPI
page_width = 4400   # 25 inches at 100 DPI
page_height = 1300  # 13 inches at 100 DPI

print("Generating RGB swatches...")
rgb_swatches = generate_rgb_swatches(num_swatches)

print("Creating swatch image...")
swatch_image = create_swatch_image(rgb_swatches, swatch_size, page_width, page_height)

print("Saving the image...")
plt.figure(figsize=(44, 13), dpi=300)
plt.imshow(swatch_image)
plt.axis('off')
plt.savefig('rgb_swatches.png', bbox_inches='tight', pad_inches=0)
plt.close()

print(f"Generated {len(rgb_swatches)} RGB swatches.")
print("Swatch image saved as 'rgb_swatches.png'")

print("Saving RGB values to CSV...")
np.savetxt('rgb_values.csv', rgb_swatches, delimiter=',', header='R,G,B', comments='')
print("RGB values saved to 'rgb_values.csv'")