#!/usr/bin/env python3
"""
Mask knobs using edge detection and contour finding
Traces the actual boundary based on contrast edges
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage
import cv2

def find_knob_contour(img_array, canny_low=50, canny_high=150):
    """
    Find the knob contour using edge detection

    Args:
        img_array: Input image as numpy array
        canny_low: Lower threshold for Canny edge detection
        canny_high: Upper threshold for Canny edge detection

    Returns:
        Largest contour or None
    """
    # Convert to grayscale
    gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)

    # Apply Gaussian blur to reduce noise
    blurred = cv2.GaussianBlur(gray, (5, 5), 0)

    # Canny edge detection
    edges = cv2.Canny(blurred, canny_low, canny_high)

    # Dilate edges slightly to close small gaps
    kernel = np.ones((3, 3), np.uint8)
    edges = cv2.dilate(edges, kernel, iterations=1)

    # Find contours
    contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

    if not contours:
        return None

    # Get the largest contour (should be the knob)
    largest_contour = max(contours, key=cv2.contourArea)

    return largest_contour

def create_contour_mask(img_shape, contour, edge_feather=5):
    """
    Create mask from contour with smooth edges

    Args:
        img_shape: (height, width) of image
        contour: OpenCV contour
        edge_feather: Pixels to feather at edge

    Returns:
        Alpha channel as uint8 array
    """
    height, width = img_shape[:2]

    # Create binary mask from contour
    mask_binary = np.zeros((height, width), dtype=np.uint8)
    cv2.drawContours(mask_binary, [contour], -1, 255, thickness=cv2.FILLED)

    # Apply morphological closing to fill any gaps
    kernel = np.ones((5, 5), np.uint8)
    mask_binary = cv2.morphologyEx(mask_binary, cv2.MORPH_CLOSE, kernel)

    # Smooth edges with Gaussian blur
    if edge_feather > 0:
        mask_float = mask_binary.astype(float) / 255.0
        mask_smooth = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_smooth * 255).astype(np.uint8)
    else:
        alpha = mask_binary

    return alpha

def process_image_with_edge_detection(image_path, output_path=None, canny_low=50, canny_high=150, edge_feather=5):
    """Process image using edge detection"""
    print(f"Processing: {Path(image_path).name}")

    # Load image
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    # Find contour
    print(f"   Detecting edges (Canny: {canny_low}/{canny_high})...")
    contour = find_knob_contour(img_array, canny_low, canny_high)

    if contour is None:
        print("   ❌ Could not detect knob boundary")
        print("   💡 Try adjusting Canny thresholds")
        return None

    # Calculate contour properties
    area = cv2.contourArea(contour)
    perimeter = cv2.arcLength(contour, True)
    circularity = 4 * np.pi * area / (perimeter * perimeter) if perimeter > 0 else 0

    print(f"   ✅ Contour detected: area={area:.0f}px², circularity={circularity:.2f}")

    # Create mask from contour
    alpha = create_contour_mask(img_array.shape, contour, edge_feather)

    # Calculate coverage
    coverage = np.sum(alpha > 0) / alpha.size * 100
    print(f"   ✅ Mask coverage: {coverage:.1f}%")

    # Create RGBA image
    rgba_array = np.dstack([img_array, alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    # Determine output path
    if output_path is None:
        output_path = Path(image_path).parent / f"{Path(image_path).stem}_masked_edge.png"

    # Save
    rgba_img.save(output_path, 'PNG')
    print(f"   ✅ Saved: {output_path}\n")

    return output_path

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 mask_with_edge_detection.py <input_image> [canny_low] [canny_high] [edge_feather]")
        print("\nParameters:")
        print("  canny_low: Lower Canny threshold (default: 50)")
        print("  canny_high: Upper Canny threshold (default: 150)")
        print("  edge_feather: Pixels to feather at edge (default: 5)")
        print("\nExample: python3 mask_with_edge_detection.py series_3_1.png 30 120 8")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    canny_low = int(sys.argv[2]) if len(sys.argv) > 2 else 50
    canny_high = int(sys.argv[3]) if len(sys.argv) > 3 else 150
    edge_feather = int(sys.argv[4]) if len(sys.argv) > 4 else 5

    # Check if file exists
    if not input_path.exists():
        # Try in Desktop/knobs
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path.name
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print("🎯 EDGE DETECTION MASKING")
    print("=" * 60)

    try:
        output_path = process_image_with_edge_detection(
            input_path,
            canny_low=canny_low,
            canny_high=canny_high,
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
