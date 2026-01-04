#!/usr/bin/env python3
"""
Batch process all hero knobs using GrabCut algorithm
"""
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage
import cv2

KNOBS_SOURCE = Path.home() / 'Desktop' / 'knobs'
HERO_WORKSPACE = Path.home() / 'Documents' / '3_Development' / 'Repos' / 'materialize' / 'input' / 'hero_knobs'

# Selected series
SELECTED_SERIES = [
    ('45087b68-2047-4ab1-a014-42daee1a2162', 'series_1'),
    ('996dd52e-4f61-4a39-8c84-1633a8a92707', 'series_2'),
    ('855f2c8b-55f8-48c8-9a4e-be48d5e15d06', 'series_3'),
]

def segment_with_grabcut(img_array, rect_margin=50, iterations=5):
    """Segment foreground using GrabCut"""
    height, width = img_array.shape[:2]
    rect = (rect_margin, rect_margin, width - 2 * rect_margin, height - 2 * rect_margin)

    mask = np.zeros((height, width), np.uint8)
    bgd_model = np.zeros((1, 65), np.float64)
    fgd_model = np.zeros((1, 65), np.float64)

    cv2.grabCut(img_array, mask, rect, bgd_model, fgd_model, iterations, cv2.GC_INIT_WITH_RECT)
    mask2 = np.where((mask == 2) | (mask == 0), 0, 1).astype('uint8')

    return mask2

def refine_mask(mask_binary, min_size=1000, edge_feather=6):
    """Refine binary mask"""
    # Remove small regions
    labeled, num_features = ndimage.label(mask_binary)
    if num_features > 1:
        sizes = ndimage.sum(mask_binary, labeled, range(num_features + 1))
        mask_sizes = sizes > min_size
        remove_pixel = ~mask_sizes[labeled]
        mask_binary = mask_binary.copy()
        mask_binary[remove_pixel] = 0

    # Morphological operations
    kernel = np.ones((5, 5), np.uint8)
    mask_binary = cv2.morphologyEx(mask_binary, cv2.MORPH_CLOSE, kernel, iterations=2)
    mask_binary = cv2.morphologyEx(mask_binary, cv2.MORPH_OPEN, kernel, iterations=1)
    mask_binary = ndimage.binary_fill_holes(mask_binary).astype(np.uint8)

    # Smooth edges
    if edge_feather > 0:
        mask_float = mask_binary.astype(float)
        mask_smooth = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_smooth * 255).astype(np.uint8)
    else:
        alpha = mask_binary * 255

    return alpha

def process_series(uuid, series_name):
    """Process all images in a series"""
    print(f"\n📦 {series_name.upper()}: {uuid}")

    # Create series directory
    series_dir = HERO_WORKSPACE / series_name
    series_dir.mkdir(parents=True, exist_ok=True)

    # Find all images
    pattern = f"stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_{uuid}_*.png"
    source_files = sorted(KNOBS_SOURCE.glob(pattern))

    if not source_files:
        print(f"   ⚠️  No files found")
        return

    print(f"   Found {len(source_files)} variations")

    for src_file in source_files:
        variation = src_file.name.split('_')[-1]
        dst_name = f"{series_name}_{variation}"
        dst_path = series_dir / dst_name

        try:
            # Load image
            img = Image.open(src_file).convert('RGB')
            img_array = np.array(img)

            # GrabCut segmentation
            mask_binary = segment_with_grabcut(img_array, rect_margin=50, iterations=5)

            # Refine mask
            alpha = refine_mask(mask_binary, min_size=1000, edge_feather=6)

            # Create RGBA
            rgba_array = np.dstack([img_array, alpha])
            rgba_img = Image.fromarray(rgba_array, 'RGBA')

            # Save
            rgba_img.save(dst_path, 'PNG')

            # Stats
            coverage = np.sum(alpha > 0) / alpha.size * 100
            size_kb = dst_path.stat().st_size / 1024
            print(f"   ✅ {dst_name}: coverage={coverage:.1f}%, size={size_kb:.0f}KB")

        except Exception as e:
            print(f"   ❌ {dst_name}: {e}")

def main():
    print("🎯 BATCH MASKING - GRABCUT METHOD")
    print("=" * 80)
    print(f"Source: {KNOBS_SOURCE}")
    print(f"Output: {HERO_WORKSPACE}")

    # Process all series
    for uuid, series_name in SELECTED_SERIES:
        process_series(uuid, series_name)

    print("\n" + "=" * 80)
    print("✅ BATCH MASKING COMPLETE")
    print("=" * 80)
    print(f"\n📁 Output: {HERO_WORKSPACE}")
    print(f"\n💡 Next: Run materialize pipeline")
    print(f"   cd ~/Documents/3_Development/Repos/materialize")
    print(f"   python -m materialize --in input/hero_knobs --out dist/hero_knobs")

if __name__ == '__main__':
    main()
