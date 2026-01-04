#!/usr/bin/env python3
"""
Re-mask all hero knobs using improved color distance method
Handles shadows, off-white backgrounds, and gradients
"""
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage

KNOBS_SOURCE = Path.home() / 'Desktop' / 'knobs'
HERO_WORKSPACE = Path.home() / 'Documents' / '3_Development' / 'Repos' / 'materialize' / 'input' / 'hero_knobs'

# Selected series
SELECTED_SERIES = [
    ('45087b68-2047-4ab1-a014-42daee1a2162', 'series_1'),
    ('996dd52e-4f61-4a39-8c84-1633a8a92707', 'series_2'),
    ('855f2c8b-55f8-48c8-9a4e-be48d5e15d06', 'series_3'),
]

def sample_background_color(img_array, sample_size=20):
    """Sample background color from corners"""
    h, w = img_array.shape[:2]
    corners = [
        img_array[:sample_size, :sample_size],
        img_array[:sample_size, -sample_size:],
        img_array[-sample_size:, :sample_size],
        img_array[-sample_size:, -sample_size:]
    ]
    corner_pixels = np.concatenate([c.reshape(-1, 3) for c in corners])
    return np.median(corner_pixels, axis=0)

def create_color_distance_mask(img_array, bg_color, color_threshold=40, min_brightness=180, edge_feather=3):
    """Create mask using color distance from background"""

    # Color distance from background
    color_diff = img_array - bg_color[np.newaxis, np.newaxis, :]
    color_distance = np.sqrt(np.sum(color_diff ** 2, axis=2))

    # Brightness
    brightness = np.mean(img_array, axis=2)

    # Create mask
    mask = (color_distance > color_threshold) | (brightness < min_brightness)

    # Clean up
    structure = np.ones((5, 5))
    mask = ndimage.binary_closing(mask, structure=structure, iterations=2)
    mask = ndimage.binary_fill_holes(mask)

    # Keep only largest region
    labeled, num_features = ndimage.label(mask)
    if num_features > 1:
        sizes = ndimage.sum(mask, labeled, range(num_features + 1))
        max_label = np.argmax(sizes[1:]) + 1
        mask = labeled == max_label

    # Dilate slightly
    mask = ndimage.binary_dilation(mask, structure=np.ones((3, 3)))

    # Smooth edges
    if edge_feather > 0:
        mask_float = mask.astype(float)
        mask_blurred = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_blurred * 255).astype(np.uint8)
    else:
        alpha = (mask * 255).astype(np.uint8)

    return alpha

def process_series(uuid, series_name):
    """Process all images in a series"""
    print(f"\n📦 {series_name.upper()}: {uuid}")

    # Create series directory
    series_dir = HERO_WORKSPACE / series_name
    series_dir.mkdir(parents=True, exist_ok=True)

    # Find all images in this series
    pattern = f"stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_{uuid}_*.png"
    source_files = sorted(KNOBS_SOURCE.glob(pattern))

    if not source_files:
        print(f"   ⚠️  No files found")
        return

    print(f"   Found {len(source_files)} variations")

    for src_file in source_files:
        variation = src_file.name.split('_')[-1]  # _0.png, _1.png, etc.
        dst_name = f"{series_name}_{variation}"
        dst_path = series_dir / dst_name

        try:
            # Load image
            img = Image.open(src_file).convert('RGB')
            img_array = np.array(img, dtype=np.float32)

            # Sample background
            bg_color = sample_background_color(img_array)

            # Create mask
            alpha = create_color_distance_mask(
                img_array,
                bg_color,
                color_threshold=40,
                min_brightness=180,
                edge_feather=3
            )

            # Create RGBA
            rgba_array = np.dstack([img_array.astype(np.uint8), alpha])
            rgba_img = Image.fromarray(rgba_array, 'RGBA')

            # Save
            rgba_img.save(dst_path, 'PNG')

            # Stats
            coverage = np.sum(alpha > 0) / alpha.size * 100
            print(f"   ✅ {dst_name}: BG=RGB({bg_color[0]:.0f},{bg_color[1]:.0f},{bg_color[2]:.0f}), coverage={coverage:.1f}%")

        except Exception as e:
            print(f"   ❌ {dst_name}: {e}")

def main():
    print("🎭 RE-MASKING ALL HERO KNOBS (Color Distance Method)")
    print("=" * 80)
    print(f"Source: {KNOBS_SOURCE}")
    print(f"Output: {HERO_WORKSPACE}")

    # Process all series
    for uuid, series_name in SELECTED_SERIES:
        process_series(uuid, series_name)

    print("\n" + "=" * 80)
    print("✅ RE-MASKING COMPLETE")
    print("=" * 80)
    print(f"\n📁 Output: {HERO_WORKSPACE}")
    print(f"\n💡 Next: Run materialize pipeline")
    print(f"   cd ~/Documents/3_Development/Repos/materialize")
    print(f"   python -m materialize --in input/hero_knobs --out dist/hero_knobs")

if __name__ == '__main__':
    main()
