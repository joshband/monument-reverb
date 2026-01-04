#!/usr/bin/env python3
"""Preview knob composite layers before building the plugin.

Usage:
    python3 scripts/preview_knob_composite.py [--rotation DEGREES]

Example:
    python3 scripts/preview_knob_composite.py --rotation 45
"""

import sys
import argparse
from pathlib import Path
from PIL import Image, ImageDraw
import math

def rotate_image(image, angle_degrees):
    """Rotate image around its center, preserving alpha."""
    return image.rotate(angle_degrees, resample=Image.BICUBIC, expand=False)

def composite_knob_layers(base_dir, rotation_degrees=0):
    """Composite all 4 knob layers with optional rotation."""
    base_dir = Path(base_dir)

    # Load all 4 layers
    layers = {
        'base': base_dir / 'base_body_concrete.png',
        'ring': base_dir / 'detail_ring_engraved.png',
        'indicator': base_dir / 'indicator_metal.png',
        'cap': base_dir / 'center_cap_brushed_metal.png'
    }

    # Verify all layers exist
    for name, path in layers.items():
        if not path.exists():
            print(f"❌ Missing layer: {path}")
            sys.exit(1)

    # Load images
    base = Image.open(layers['base']).convert('RGBA')
    ring = Image.open(layers['ring']).convert('RGBA')
    indicator = Image.open(layers['indicator']).convert('RGBA')
    cap = Image.open(layers['cap']).convert('RGBA')

    # Get dimensions (all should be same size)
    width, height = base.size
    print(f"📐 Layer size: {width}×{height}")

    # Create blank canvas
    composite = Image.new('RGBA', (width, height), (0, 0, 0, 0))

    # Layer 0: Base body (rotates)
    rotated_base = rotate_image(base, -rotation_degrees)
    composite = Image.alpha_composite(composite, rotated_base)
    print(f"✅ Layer 0: Base body (rotated {rotation_degrees}°)")

    # Layer 1: Detail ring (static)
    composite = Image.alpha_composite(composite, ring)
    print(f"✅ Layer 1: Detail ring (static)")

    # Layer 2: Indicator (rotates)
    rotated_indicator = rotate_image(indicator, -rotation_degrees)
    composite = Image.alpha_composite(composite, rotated_indicator)
    print(f"✅ Layer 2: Indicator (rotated {rotation_degrees}°)")

    # Layer 3: Center cap (static)
    composite = Image.alpha_composite(composite, cap)
    print(f"✅ Layer 3: Center cap (static)")

    return composite

def main():
    parser = argparse.ArgumentParser(description='Preview knob composite')
    parser.add_argument('--rotation', type=float, default=0,
                       help='Rotation angle in degrees (default: 0)')
    parser.add_argument('--out', type=str, default=None,
                       help='Output path (default: show in preview)')
    parser.add_argument('--dir', type=str,
                       default='assets/ui/knobs_test',
                       help='Knob layers directory')

    args = parser.parse_args()

    print(f"🎨 Compositing knob layers from: {args.dir}")
    print(f"🔄 Rotation: {args.rotation}°")
    print()

    composite = composite_knob_layers(args.dir, args.rotation)

    if args.out:
        output_path = Path(args.out)
        output_path.parent.mkdir(parents=True, exist_ok=True)
        composite.save(output_path)
        print(f"\n💾 Saved to: {output_path}")
    else:
        print("\n👁️  Opening preview...")
        composite.show()

if __name__ == '__main__':
    main()
