#!/usr/bin/env python3
"""
Mask knobs using GrabCut algorithm
Automatic foreground/background segmentation with refinement
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage
import cv2

def segment_with_grabcut(img_array, rect_margin=50, iterations=5):
    """
    Segment foreground using GrabCut algorithm

    Args:
        img_array: Input image as numpy array (RGB)
        rect_margin: Margin from edges for initial rectangle
        iterations: Number of GrabCut iterations

    Returns:
        Foreground mask as boolean array
    """
    height, width = img_array.shape[:2]

    # Initialize rectangle (initial estimate of foreground region)
    rect = (rect_margin, rect_margin, width - 2 * rect_margin, height - 2 * rect_margin)

    # Initialize masks for GrabCut
    mask = np.zeros((height, width), np.uint8)
    bgd_model = np.zeros((1, 65), np.float64)
    fgd_model = np.zeros((1, 65), np.float64)

    # Run GrabCut
    cv2.grabCut(img_array, mask, rect, bgd_model, fgd_model, iterations, cv2.GC_INIT_WITH_RECT)

    # Create binary mask (foreground = 1, 3; background = 0, 2)
    mask2 = np.where((mask == 2) | (mask == 0), 0, 1).astype('uint8')

    return mask2

def refine_mask(mask_binary, min_size=1000, edge_feather=5):
    """
    Refine the binary mask

    Args:
        mask_binary: Binary mask (0 or 1)
        min_size: Minimum region size to keep
        edge_feather: Gaussian blur sigma for smooth edges

    Returns:
        Refined alpha channel as uint8
    """
    # Remove small regions
    labeled, num_features = ndimage.label(mask_binary)
    if num_features > 1:
        sizes = ndimage.sum(mask_binary, labeled, range(num_features + 1))
        # Keep only regions larger than min_size
        mask_sizes = sizes > min_size
        remove_pixel = ~mask_sizes[labeled]
        mask_binary = mask_binary.copy()
        mask_binary[remove_pixel] = 0

    # Morphological operations to smooth
    kernel = np.ones((5, 5), np.uint8)
    mask_binary = cv2.morphologyEx(mask_binary, cv2.MORPH_CLOSE, kernel, iterations=2)
    mask_binary = cv2.morphologyEx(mask_binary, cv2.MORPH_OPEN, kernel, iterations=1)

    # Fill holes
    mask_binary = ndimage.binary_fill_holes(mask_binary).astype(np.uint8)

    # Smooth edges
    if edge_feather > 0:
        mask_float = mask_binary.astype(float)
        mask_smooth = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_smooth * 255).astype(np.uint8)
    else:
        alpha = mask_binary * 255

    return alpha

def process_image_with_grabcut(image_path, output_path=None, rect_margin=50, iterations=5, edge_feather=6):
    """Process image using GrabCut"""
    print(f"Processing: {Path(image_path).name}")

    # Load image
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    print(f"   Running GrabCut (iterations={iterations}, margin={rect_margin}px)...")

    # Segment with GrabCut
    mask_binary = segment_with_grabcut(img_array, rect_margin, iterations)

    # Refine mask
    print("   Refining mask...")
    alpha = refine_mask(mask_binary, min_size=1000, edge_feather=edge_feather)

    # Calculate coverage
    coverage = np.sum(alpha > 0) / alpha.size * 100
    print(f"   ✅ Mask coverage: {coverage:.1f}%")

    # Create RGBA image
    rgba_array = np.dstack([img_array, alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    # Determine output path
    if output_path is None:
        output_path = Path(image_path).parent / f"{Path(image_path).stem}_masked_grabcut.png"

    # Save
    rgba_img.save(output_path, 'PNG')
    print(f"   ✅ Saved: {output_path}\n")

    return output_path

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 mask_with_grabcut.py <input_image> [rect_margin] [iterations] [edge_feather]")
        print("\nParameters:")
        print("  rect_margin: Margin from edges for initial rect (default: 50)")
        print("  iterations: Number of GrabCut iterations (default: 5)")
        print("  edge_feather: Pixels to feather at edge (default: 6)")
        print("\nExample: python3 mask_with_grabcut.py series_3_1.png 80 8 8")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    rect_margin = int(sys.argv[2]) if len(sys.argv) > 2 else 50
    iterations = int(sys.argv[3]) if len(sys.argv) > 3 else 5
    edge_feather = int(sys.argv[4]) if len(sys.argv) > 4 else 6

    # Check if file exists
    if not input_path.exists():
        # Try in Desktop/knobs
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path.name
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print("🎯 GRABCUT SEGMENTATION MASKING")
    print("=" * 60)

    try:
        output_path = process_image_with_grabcut(
            input_path,
            rect_margin=rect_margin,
            iterations=iterations,
            edge_feather=edge_feather
        )

        if output_path:
            print(f"To preview: open {output_path}")

    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
