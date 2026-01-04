#!/usr/bin/env python3
"""
Advanced masking using color distance from background
Handles shadows, off-white backgrounds, and gradients
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage

def sample_background_color(img_array, sample_size=20):
    """
    Sample the background color from image corners

    Returns:
        Average background color as [R, G, B]
    """
    h, w = img_array.shape[:2]

    # Sample from all four corners
    corners = [
        img_array[:sample_size, :sample_size],  # Top-left
        img_array[:sample_size, -sample_size:],  # Top-right
        img_array[-sample_size:, :sample_size],  # Bottom-left
        img_array[-sample_size:, -sample_size:]  # Bottom-right
    ]

    # Calculate average color from corners
    corner_pixels = np.concatenate([c.reshape(-1, 3) for c in corners])
    bg_color = np.median(corner_pixels, axis=0)

    return bg_color

def create_color_distance_mask(image_path, color_threshold=40, min_brightness=180, edge_feather=3):
    """
    Create mask based on color distance from background

    Args:
        image_path: Path to input image
        color_threshold: Maximum color distance to consider background (Euclidean distance)
        min_brightness: Minimum brightness to be considered background
        edge_feather: Gaussian blur sigma for smooth edges

    Returns:
        PIL Image with alpha channel, background color
    """
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img, dtype=np.float32)

    # Sample background color
    bg_color = sample_background_color(img_array)

    print(f"   Background color: RGB({bg_color[0]:.0f}, {bg_color[1]:.0f}, {bg_color[2]:.0f})")

    # Calculate color distance from background (Euclidean distance in RGB space)
    color_diff = img_array - bg_color[np.newaxis, np.newaxis, :]
    color_distance = np.sqrt(np.sum(color_diff ** 2, axis=2))

    # Calculate brightness
    brightness = np.mean(img_array, axis=2)

    # Create mask: keep pixels that are:
    # 1. Far from background color (color_distance > threshold), OR
    # 2. Dark enough (brightness < min_brightness)
    mask = (color_distance > color_threshold) | (brightness < min_brightness)

    # Morphological operations to clean up mask
    structure = np.ones((5, 5))
    mask = ndimage.binary_closing(mask, structure=structure, iterations=2)
    mask = ndimage.binary_fill_holes(mask)

    # Remove small isolated regions
    labeled, num_features = ndimage.label(mask)
    if num_features > 1:
        sizes = ndimage.sum(mask, labeled, range(num_features + 1))
        # Keep only the largest region (the knob)
        max_label = np.argmax(sizes[1:]) + 1
        mask = labeled == max_label

    # Dilate slightly to ensure edges are captured
    mask = ndimage.binary_dilation(mask, structure=np.ones((3, 3)))

    # Smooth edges
    if edge_feather > 0:
        mask_float = mask.astype(float)
        mask_blurred = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_blurred * 255).astype(np.uint8)
    else:
        alpha = (mask * 255).astype(np.uint8)

    # Create RGBA image
    rgba_array = np.dstack([img_array.astype(np.uint8), alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    return rgba_img, mask, bg_color

def process_image(input_path, output_path=None, color_threshold=40, min_brightness=180):
    """Process a single image"""
    print(f"Processing: {Path(input_path).name}")
    print(f"   Color threshold: {color_threshold}")
    print(f"   Min brightness: {min_brightness}")

    masked_img, mask_array, bg_color = create_color_distance_mask(
        input_path,
        color_threshold=color_threshold,
        min_brightness=min_brightness
    )

    # Calculate mask coverage
    coverage = np.sum(mask_array) / mask_array.size * 100

    # Determine output path
    if output_path is None:
        output_path = Path(input_path).parent / f"{Path(input_path).stem}_masked_color.png"

    # Save
    masked_img.save(output_path, 'PNG')

    print(f"   ✅ Mask coverage: {coverage:.1f}%")
    print(f"   ✅ Saved: {output_path}\n")

    return output_path

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 mask_with_color_distance.py <input_image> [color_threshold] [min_brightness]")
        print("\nParameters:")
        print("  color_threshold: Color distance threshold (default: 40)")
        print("  min_brightness: Minimum brightness for background (default: 180)")
        print("\nExample: python3 mask_with_color_distance.py series_3_1.png 35 170")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    color_threshold = int(sys.argv[2]) if len(sys.argv) > 2 else 40
    min_brightness = int(sys.argv[3]) if len(sys.argv) > 3 else 180

    # Check if file exists
    if not input_path.exists():
        # Try in Desktop/knobs
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path.name
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print("🎭 COLOR DISTANCE MASKING")
    print("=" * 60)

    try:
        output_path = process_image(
            input_path,
            color_threshold=color_threshold,
            min_brightness=min_brightness
        )

        print(f"To preview: open {output_path}")

    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
