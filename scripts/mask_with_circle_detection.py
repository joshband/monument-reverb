#!/usr/bin/env python3
"""
Mask knobs using circle detection
Finds the circular boundary and creates a clean circular mask
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage
import cv2

def detect_knob_circle(img_array, min_radius_ratio=0.35, max_radius_ratio=0.48):
    """
    Detect the circular boundary of the knob

    Args:
        img_array: Input image as numpy array
        min_radius_ratio: Minimum radius as ratio of image size
        max_radius_ratio: Maximum radius as ratio of image size

    Returns:
        (center_x, center_y, radius) or None
    """
    # Convert to grayscale
    gray = cv2.cvtColor(img_array, cv2.COLOR_RGB2GRAY)

    # Apply Canny edge detection
    edges = cv2.Canny(gray, 50, 150, apertureSize=3)

    # Calculate radius range
    height, width = gray.shape
    min_size = min(height, width)
    min_radius = int(min_size * min_radius_ratio)
    max_radius = int(min_size * max_radius_ratio)

    # Detect circles using Hough Circle Transform
    circles = cv2.HoughCircles(
        gray,
        cv2.HOUGH_GRADIENT,
        dp=1,
        minDist=min_size // 2,
        param1=100,
        param2=30,
        minRadius=min_radius,
        maxRadius=max_radius
    )

    if circles is not None:
        # Get the first (strongest) circle
        circles = np.uint16(np.around(circles))
        x, y, r = circles[0, 0]
        return (x, y, r)

    return None

def create_circular_mask(img_shape, center_x, center_y, radius, edge_feather=3):
    """
    Create a circular mask with smooth edges

    Args:
        img_shape: (height, width) of image
        center_x, center_y: Circle center
        radius: Circle radius
        edge_feather: Pixels to feather at edge

    Returns:
        Alpha channel as uint8 array
    """
    height, width = img_shape[:2]

    # Create coordinate grids
    y, x = np.ogrid[:height, :width]

    # Calculate distance from center
    distance = np.sqrt((x - center_x)**2 + (y - center_y)**2)

    # Create mask with smooth falloff at edge
    if edge_feather > 0:
        # Smooth transition zone
        inner_radius = radius - edge_feather
        outer_radius = radius + edge_feather

        # Full opacity inside inner_radius
        mask = np.ones((height, width), dtype=float)

        # Smooth falloff in transition zone
        transition = (distance > inner_radius) & (distance < outer_radius)
        mask[transition] = 1.0 - (distance[transition] - inner_radius) / (outer_radius - inner_radius)

        # Transparent outside outer_radius
        mask[distance >= outer_radius] = 0.0

        alpha = (mask * 255).astype(np.uint8)
    else:
        # Hard edge
        alpha = np.zeros((height, width), dtype=np.uint8)
        alpha[distance <= radius] = 255

    return alpha

def process_image_with_circle_detection(image_path, output_path=None, edge_feather=5):
    """Process image using circle detection"""
    print(f"Processing: {Path(image_path).name}")

    # Load image
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    # Detect circle
    print("   Detecting circular boundary...")
    circle = detect_knob_circle(img_array)

    if circle is None:
        print("   ❌ Could not detect circular boundary")
        print("   💡 Try manually specifying center and radius")
        return None

    center_x, center_y, radius = circle
    print(f"   ✅ Circle detected: center=({center_x}, {center_y}), radius={radius}px")

    # Create circular mask
    alpha = create_circular_mask(img_array.shape, center_x, center_y, radius, edge_feather)

    # Calculate coverage
    coverage = np.sum(alpha > 0) / alpha.size * 100
    print(f"   ✅ Mask coverage: {coverage:.1f}%")

    # Create RGBA image
    rgba_array = np.dstack([img_array, alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    # Determine output path
    if output_path is None:
        output_path = Path(image_path).parent / f"{Path(image_path).stem}_masked_circle.png"

    # Save
    rgba_img.save(output_path, 'PNG')
    print(f"   ✅ Saved: {output_path}\n")

    return output_path

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 mask_with_circle_detection.py <input_image> [edge_feather]")
        print("\nParameters:")
        print("  edge_feather: Pixels to feather at edge (default: 5)")
        print("\nExample: python3 mask_with_circle_detection.py series_3_1.png 8")
        sys.exit(1)

    input_path = Path(sys.argv[1])
    edge_feather = int(sys.argv[2]) if len(sys.argv) > 2 else 5

    # Check if file exists
    if not input_path.exists():
        # Try in Desktop/knobs
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path.name
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print("🎯 CIRCLE DETECTION MASKING")
    print("=" * 60)
    print(f"Edge feather: {edge_feather}px\n")

    try:
        output_path = process_image_with_circle_detection(input_path, edge_feather=edge_feather)

        if output_path:
            print(f"To preview: open {output_path}")

    except Exception as e:
        print(f"❌ Error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == '__main__':
    main()
