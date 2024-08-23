import numpy as np
import matplotlib.pyplot as plt
from colormath.color_objects import sRGBColor, LabColor
from colormath.color_conversions import convert_color
from tqdm import tqdm
import colorsys
import math

def rgb_to_lab(rgb):
    return convert_color(sRGBColor(*rgb), LabColor).get_value_tuple()

def lab_to_rgb(lab):
    rgb = convert_color(LabColor(*lab), sRGBColor)
    return np.clip(rgb.get_value_tuple(), 0, 1)

def generate_base_colors():
    # Generate a range of hues
    hues = np.linspace(0, 1, 18, endpoint=False)
    
    colors = []
    for h in hues:
        # Add a saturated version
        colors.append(colorsys.hsv_to_rgb(h, 1.0, 1.0))
        # Add a less saturated version
        colors.append(colorsys.hsv_to_rgb(h, 0.5, 1.0))
        # Add a darker version
        colors.append(colorsys.hsv_to_rgb(h, 1.0, 0.5))
    
    # Add grayscale
    grays = np.linspace(0, 1, 10)
    colors.extend([[g, g, g] for g in grays])
    
    # Add some earth tones and pastel colors
    additional_colors = [
        [0.76, 0.60, 0.42],  # Tan
        [0.55, 0.27, 0.07],  # Brown
        [0.33, 0.42, 0.18],  # Olive
        [0.85, 0.65, 0.13],  # Golden
        [0.55, 0.47, 0.37],  # Taupe
        [0.93, 0.77, 0.80],  # Pastel Pink
        [0.80, 0.92, 0.77],  # Pastel Green
        [0.77, 0.83, 0.93],  # Pastel Blue
        [0.96, 0.87, 0.70],  # Pastel Yellow
        [0.85, 0.77, 0.93],  # Pastel Purple
    ]
    colors.extend(additional_colors)
    
    return np.array(colors)

def generate_color_variations(base_colors, variations_per_color=5):
    variations = []
    for color in base_colors:
        lab_color = rgb_to_lab(color)
        for _ in range(variations_per_color):
            # Randomly adjust L, a, and b values
            new_lab = [
                np.clip(lab_color[0] + np.random.normal(0, 10), 0, 100),
                np.clip(lab_color[1] + np.random.normal(0, 10), -128, 127),
                np.clip(lab_color[2] + np.random.normal(0, 10), -128, 127)
            ]
            new_rgb = lab_to_rgb(new_lab)
            variations.append(new_rgb)
    return np.array(variations)

def delta_e_cie2000(lab1, lab2, kL=1, kC=1, kH=1):
    """
    Calculates the Delta E (CIE2000) of two colors in the L*a*b* color space.
    """
    L1, a1, b1 = lab1
    L2, a2, b2 = lab2
    
    C1 = math.sqrt(a1**2 + b1**2)
    C2 = math.sqrt(a2**2 + b2**2)
    C_avg = (C1 + C2) / 2
    
    G = 0.5 * (1 - math.sqrt(C_avg**7 / (C_avg**7 + 25**7)))
    
    a1_prime = a1 * (1 + G)
    a2_prime = a2 * (1 + G)
    
    C1_prime = math.sqrt(a1_prime**2 + b1**2)
    C2_prime = math.sqrt(a2_prime**2 + b2**2)
    C_avg_prime = (C1_prime + C2_prime) / 2
    
    h1_prime = math.atan2(b1, a1_prime) % (2 * math.pi)
    h2_prime = math.atan2(b2, a2_prime) % (2 * math.pi)
    
    H_avg_prime = (h1_prime + h2_prime) / 2 if abs(h1_prime - h2_prime) <= math.pi else (h1_prime + h2_prime + 2 * math.pi) / 2
    T = (1 - 0.17 * math.cos(H_avg_prime - math.pi/6) + 0.24 * math.cos(2 * H_avg_prime) + 
         0.32 * math.cos(3 * H_avg_prime + math.pi/30) - 0.20 * math.cos(4 * H_avg_prime - 21*math.pi/60))
    
    delta_h_prime = h2_prime - h1_prime
    delta_H_prime = 2 * math.sqrt(C1_prime * C2_prime) * math.sin(delta_h_prime / 2)
    
    L_avg = (L1 + L2) / 2
    SL = 1 + ((0.015 * (L_avg - 50)**2) / math.sqrt(20 + (L_avg - 50)**2))
    SC = 1 + 0.045 * C_avg_prime
    SH = 1 + 0.015 * C_avg_prime * T
    
    RT = (-2 * math.sqrt(C_avg_prime**7 / (C_avg_prime**7 + 25**7)) * 
          math.sin(60 * math.exp(-((H_avg_prime - 275*math.pi/180) / (25*math.pi/180))**2)))
    
    delta_L_prime = L2 - L1
    delta_C_prime = C2_prime - C1_prime
    
    dL = delta_L_prime / (kL * SL)
    dC = delta_C_prime / (kC * SC)
    dH = delta_H_prime / (kH * SH)
    
    return math.sqrt(dL**2 + dC**2 + dH**2 + RT * dC * dH)

def select_diverse_colors(colors, num_swatches):
    lab_colors = np.array([rgb_to_lab(rgb) for rgb in colors])
    
    selected_indices = [0]  # Start with the first color
    pbar = tqdm(total=min(num_swatches, len(colors)) - 1)
    while len(selected_indices) < num_swatches and len(selected_indices) < len(colors):
        last_selected = lab_colors[selected_indices[-1]]
        color_diffs = np.array([delta_e_cie2000(last_selected, lab2) for lab2 in lab_colors])
        color_diffs[selected_indices] = -1  # Exclude already selected colors
        max_diff_index = np.argmax(color_diffs)
        selected_indices.append(max_diff_index)
        pbar.update(1)
    pbar.close()
    
    return colors[selected_indices]

def create_swatch_image(rgb_values, swatch_size, page_width, page_height):
    swatches_per_row = page_width // swatch_size
    swatches_per_col = page_height // swatch_size
    total_swatches = swatches_per_row * swatches_per_col
    
    # Convert RGB to LAB for sorting
    lab_values = np.array([rgb_to_lab(rgb) for rgb in rgb_values])
    
    # Sort by L, then A, then B
    sorted_indices = np.lexsort((lab_values[:, 2], lab_values[:, 1], lab_values[:, 0]))
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

print("Generating base colors...")
base_colors = generate_base_colors()

print("Generating color variations...")
all_colors = np.vstack([base_colors, generate_color_variations(base_colors)])

print("Selecting diverse colors...")
selected_colors = select_diverse_colors(all_colors, num_swatches)

print("Creating swatch image...")
swatch_image = create_swatch_image(selected_colors, swatch_size, page_width, page_height)

print("Saving the image...")
plt.figure(figsize=(44, 13), dpi=100)
plt.imshow(swatch_image)
plt.axis('off')
plt.savefig('practical_color_swatches.png', bbox_inches='tight', pad_inches=0)
plt.close()

print(f"Generated {len(selected_colors)} color swatches.")
print("Swatch image saved as 'practical_color_swatches.png'")

print("Saving RGB values to CSV...")
np.savetxt('practical_rgb_values.csv', 
           selected_colors, 
           delimiter=',', 
           header='R,G,B', 
           comments='')
print("RGB values saved to 'practical_rgb_values.csv'")