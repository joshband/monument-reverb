#!/usr/bin/env python3
"""
Mask oval/elliptical knobs using ellipse fitting
Handles knobs photographed at an angle
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage
import cv2

def fit_ellipse_to_knob(img_array, canny_low=15, canny_high=60):
    """
    Fit an ellipse to the knob boundary

    Args:
        img_array: Input image as numpy array
        canny_low: Lower threshold for Canny edge detection
        canny_high: Upper threshold for Canny edge detection

    Returns:
        Ellipse parameters ((cx, cy), (w, h), angle) or None
    """
    # Convert to grayscale
    gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)

    # Apply Gaussian blur
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Canny edge detection
    edges = cv2.Canny(blurred, canny_low, canny_high)

    # Find contours
    contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        return None

    # Get the largest contour
    largest_contour = max(contours, key=cv2.contourArea)

    # Need at least 5 points to fit an ellipse
    if len(largest_contour) < 5:
        return None

    # Fit ellipse to contour
    ellipse = cv2.fitEllipse(largest_contour)

    return ellipse

def create_ellipse_mask(img_shape, ellipse, edge_feather=8, expand_pixels=0):
    """
    Create mask from ellipse with smooth edges

    Args:
        img_shape: (height, width) of image
        ellipse: ((cx, cy), (w, h), angle) from cv2.fitEllipse
        edge_feather: Pixels to feather at edge
        expand_pixels: Pixels to expand ellipse (positive = larger)

    Returns:
        Alpha channel as uint8 array
    """
    height, width = img_shape[:2]

    # Unpack ellipse
    (cx, cy), (w, h), angle = ellipse

    # Expand ellipse if requested
    w += expand_pixels * 2
    h += expand_pixels * 2

    # Create binary mask
    mask = np.zeros((height, width), dtype=np.uint8)
    cv2.ellipse(mask, ((int(cx), int(cy)), (int(w), int(h)), angle), 255, thickness=cv2.FILLED)

    # Smooth edges with Gaussian blur
    if edge_feather > 0:
        mask_float = mask.astype(float) / 255.0
        mask_smooth = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_smooth * 255).astype(np.uint8)
    else:
        alpha = mask

    return alpha

def process_image_with_ellipse_fit(image_path, output_path=None, canny_low=15, canny_high=60, edge_feather=8, expand_pixels=10):
    """Process image using ellipse fitting"""
    print(f"Processing: {Path(image_path).name}")

    # Load image
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    # Fit ellipse
    print(f"   Fitting ellipse (Canny: {canny_low}/{canny_high})...")
    ellipse = fit_ellipse_to_knob(img_array, canny_low, canny_high)

    if ellipse is None:
        print("   ❌ Could not fit ellipse to boundary")
        print("   💡 Try adjusting Canny thresholds")
        return None

    # Extract ellipse parameters
    (cx, cy), (w, h), angle = ellipse
    aspect_ratio = max(w, h) / min(w, h) if min(w, h) > 0 else 1.0

    print(f"   ✅ Ellipse fitted:")
    print(f"      Center: ({cx:.0f}, {cy:.0f})")
    print(f"      Size: {w:.0f}×{h:.0f}px")
    print(f"      Angle: {angle:.1f}°")
    print(f"      Aspect ratio: {aspect_ratio:.2f}")

    # Create mask from ellipse
    alpha = create_ellipse_mask(img_array.shape, ellipse, edge_feather, expand_pixels)

    # Calculate coverage
    coverage = np.sum(alpha > 0) / alpha.size * 100
    print(f"   ✅ Mask coverage: {coverage:.1f}%")

    # Create RGBA image
    rgba_array = np.dstack([img_array, alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    # Determine output path
    if output_path is None:
        output_path = Path(image_path).parent / f"{Path(image_path).stem}_masked_ellipse.png"

    # Save
    rgba_img.save(output_path, 'PNG')
    print(f"   ✅ Saved: {output_path}\n")

    return output_path, ellipse

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 mask_with_ellipse_fit.py <input_image> [canny_low] [canny_high] [edge_feather] [expand_px]")
        print("\nParameters:")
        print("  canny_low: Lower Canny threshold (default: 15)")
        print("  canny_high: Upper Canny threshold (default: 60)")
        print("  edge_feather: Pixels to feather at edge (default: 8)")
        print("  expand_px: Pixels to expand ellipse (default: 10)")
        print("\nExample: python3 mask_with_ellipse_fit.py series_3_1.png 12 55 10 15")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    canny_low = int(sys.argv[2]) if len(sys.argv) > 2 else 15
    canny_high = int(sys.argv[3]) if len(sys.argv) > 3 else 60
    edge_feather = int(sys.argv[4]) if len(sys.argv) > 4 else 8
    expand_pixels = int(sys.argv[5]) if len(sys.argv) > 5 else 10

    # Check if file exists
    if not input_path.exists():
        # Try in Desktop/knobs
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path.name
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print("🎯 ELLIPSE FITTING MASKING")
    print("=" * 60)

    try:
        output_path, ellipse = process_image_with_ellipse_fit(
            input_path,
            canny_low=canny_low,
            canny_high=canny_high,
            edge_feather=edge_feather,
            expand_pixels=expand_pixels
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
