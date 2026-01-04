#!/usr/bin/env python3
"""
Re-mask a single knob image with adjustable parameters
For fine-tuning problematic images
"""
import sys
import numpy as np
from PIL import Image
from pathlib import Path
from scipy import ndimage

def create_smart_mask(image_path, threshold=235, edge_feather=3, morphology_size=3):
    """
    Create alpha mask with smart edge detection

    Args:
        image_path: Path to input image
        threshold: Brightness threshold (lower = more aggressive)
        edge_feather: Gaussian blur sigma for smooth edges
        morphology_size: Size for morphological operations

    Returns:
        PIL Image with alpha channel
    """
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    # Calculate brightness (weighted RGB - human perception)
    brightness = (
        0.299 * img_array[:, :, 0] +
        0.587 * img_array[:, :, 1] +
        0.114 * img_array[:, :, 2]
    )

    # Create initial mask
    mask = brightness < threshold

    # Morphological closing to fill small gaps
    if morphology_size > 0:
        structure = np.ones((morphology_size, morphology_size))
        mask = ndimage.binary_closing(mask, structure=structure)

    # Fill holes
    mask = ndimage.binary_fill_holes(mask)

    # Remove small isolated regions
    labeled, num_features = ndimage.label(mask)
    if num_features > 1:
        sizes = ndimage.sum(mask, labeled, range(num_features + 1))
        mask_size = sizes < 1000  # Remove regions smaller than 1000 pixels
        remove_pixel = mask_size[labeled]
        mask[remove_pixel] = 0

    # Smooth edges with gaussian blur
    if edge_feather > 0:
        mask_float = mask.astype(float)
        mask_blurred = ndimage.gaussian_filter(mask_float, sigma=edge_feather)
        alpha = (mask_blurred * 255).astype(np.uint8)
    else:
        alpha = (mask * 255).astype(np.uint8)

    # Create RGBA image
    rgba_array = np.dstack([img_array, alpha])
    rgba_img = Image.fromarray(rgba_array, 'RGBA')

    return rgba_img, mask

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 remask_single_knob.py <input_image> [threshold] [edge_feather]")
        print("Example: python3 remask_single_knob.py series_3_1.png 235 3")
        sys.exit(1)

    input_path = sys.argv[1]
    threshold = int(sys.argv[2]) if len(sys.argv) > 2 else 235
    edge_feather = int(sys.argv[3]) if len(sys.argv) > 3 else 3

    # Resolve paths
    if not Path(input_path).exists():
        # Try in knobs source
        input_path = Path.home() / 'Desktop' / 'knobs' / input_path
        if not input_path.exists():
            print(f"❌ File not found: {input_path}")
            sys.exit(1)

    print(f"🎭 Re-masking: {input_path.name}")
    print(f"Settings: threshold={threshold}, edge_feather={edge_feather}")

    # Process
    try:
        masked_img, mask_array = create_smart_mask(
            input_path,
            threshold=threshold,
            edge_feather=edge_feather
        )

        # Calculate mask coverage
        coverage = np.sum(mask_array) / mask_array.size * 100

        # Save output
        output_path = Path(input_path).parent / f"{Path(input_path).stem}_remasked.png"
        masked_img.save(output_path, 'PNG')

        print(f"✅ Mask coverage: {coverage:.1f}%")
        print(f"✅ Saved: {output_path}")
        print(f"\nTo preview: open {output_path}")

    except Exception as e:
        print(f"❌ Error: {e}")
        sys.exit(1)

if __name__ == '__main__':
    main()
