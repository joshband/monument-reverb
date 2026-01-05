#!/usr/bin/env python3
"""
Process codex brushed aluminum knob with proper alpha transparency.
"""

from PIL import Image, ImageDraw, ImageFilter
import numpy as np

def create_circular_mask(size):
    """Create circular alpha mask with soft edge."""
    mask = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(mask)
    margin = int(size * 0.05)
    draw.ellipse([margin, margin, size - margin, size - margin], fill=255)
    return mask.filter(ImageFilter.GaussianBlur(radius=2))

def remove_white_background(input_path, output_path):
    """Remove white background and add circular alpha."""
    img = Image.open(input_path).convert('RGB')
    img_array = np.array(img)

    # White threshold
    white_mask = np.all(img_array > 240, axis=2)
    white_mask_pil = Image.fromarray((~white_mask * 255).astype(np.uint8))

    # Combine with circular mask
    circular = create_circular_mask(img.width)
    final_mask = Image.composite(circular, Image.new('L', img.size, 0), white_mask_pil)

    # Create RGBA
    rgba = Image.new('RGBA', img.size)
    rgba.putdata([(*rgb, a) for rgb, a in zip(img.getdata(), final_mask.getdata())])
    rgba.save(output_path, 'PNG')
    print(f"✓ Created: {output_path}")

def main():
    input_path = "/Users/noisebox/Documents/3_Development/Repos/monument-reverb/assets/codex/ui_assets/monument-reverb/assets/rotary_knobs/brushed_aluminum_neutral_indicator.png"
    output_path = "assets/ui/hero_knob_pbr/albedo_rgba.png"

    print("Processing codex brushed aluminum knob...")
    remove_white_background(input_path, output_path)
    print("\nNext: Rebuild plugin")

if __name__ == "__main__":
    main()
