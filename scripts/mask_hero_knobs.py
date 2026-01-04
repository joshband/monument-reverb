#!/usr/bin/env python3
"""
Generate alpha masks for hero knobs
Removes white backgrounds and creates clean alpha channels for materialize pipeline
"""
import os
import numpy as np
from PIL import Image
from pathlib import Path

# Workspace paths
HERO_WORKSPACE = Path.home() / 'Documents' / '3_Development' / 'Repos' / 'materialize' / 'input' / 'hero_knobs'
MASKED_OUTPUT = HERO_WORKSPACE / 'masked'

def create_alpha_mask(image_path, threshold=240, edge_feather=2):
    """
    Create alpha mask from white background image

    Args:
        image_path: Path to input image
        threshold: Brightness threshold for background (0-255)
        edge_feather: Pixels to feather at edges for smooth transition

    Returns:
        PIL Image with alpha channel
    """
    img = Image.open(image_path).convert('RGB')
    img_array = np.array(img)

    # Calculate brightness (average of RGB channels)
    brightness = np.mean(img_array, axis=2)

    # Create binary mask (True = keep, False = remove)
    mask = brightness < threshold

    # Apply morphological operations to clean up mask
    from scipy import ndimage

    # Fill small holes
    mask = ndimage.binary_fill_holes(mask)

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

    return rgba_img

def process_all_images(threshold=240, edge_feather=2):
    """Process all images in workspace"""
    print("🎭 ALPHA MASKING - Hero Knobs")
    print("=" * 80)
    print(f"Workspace: {HERO_WORKSPACE}")
    print(f"Output: {MASKED_OUTPUT}\n")

    # Create output directory
    MASKED_OUTPUT.mkdir(parents=True, exist_ok=True)

    # Find all PNG images (skip manifest and already masked)
    images = [f for f in HERO_WORKSPACE.glob('series_*.png')]

    if not images:
        print("❌ No images found in workspace")
        return

    print(f"Found {len(images)} images to process\n")
    print(f"Settings: threshold={threshold}, edge_feather={edge_feather}px\n")

    # Process each image
    for img_path in sorted(images):
        filename = img_path.name
        output_path = MASKED_OUTPUT / filename

        try:
            print(f"Processing: {filename}...", end=' ')

            # Create masked version
            masked_img = create_alpha_mask(img_path, threshold, edge_feather)

            # Save
            masked_img.save(output_path, 'PNG')

            # Get file sizes for comparison
            original_size = img_path.stat().st_size / 1024  # KB
            masked_size = output_path.stat().st_size / 1024  # KB

            print(f"✅ ({original_size:.0f}KB → {masked_size:.0f}KB)")

        except Exception as e:
            print(f"❌ Error: {e}")

    print("\n" + "=" * 80)
    print("✅ MASKING COMPLETE")
    print("=" * 80)
    print(f"\n📁 Masked images: {MASKED_OUTPUT}")
    print(f"💡 Next: Review masks and process through materialize")
    print(f"\nTo preview:")
    print(f"   open {MASKED_OUTPUT}")
    print(f"\nTo run materialize:")
    print(f"   cd ~/Documents/3_Development/Repos/materialize")
    print(f"   python -m materialize --in input/hero_knobs/masked --out dist/hero_knobs")

def main():
    import sys

    # Check for scipy
    try:
        import scipy
    except ImportError:
        print("❌ scipy is required but not installed")
        print("Install with: pip3 install scipy")
        sys.exit(1)

    # Process images with default settings
    process_all_images(threshold=240, edge_feather=2)

if __name__ == '__main__':
    main()
