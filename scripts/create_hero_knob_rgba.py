#!/usr/bin/env python3
"""
Create RGBA hero knob texture by compositing albedo with primitive mask.
"""

from PIL import Image
import sys

def create_rgba_knob(albedo_path, mask_path, output_path):
    """Composite albedo (RGB) with mask (grayscale) to create RGBA texture."""

    # Load images
    albedo = Image.open(albedo_path).convert('RGB')
    mask = Image.open(mask_path).convert('L')  # Grayscale

    # Verify sizes match
    if albedo.size != mask.size:
        raise ValueError(f"Size mismatch: albedo {albedo.size} vs mask {mask.size}")

    # Create RGBA by adding mask as alpha channel
    rgba = Image.new('RGBA', albedo.size)
    rgba.putdata([
        (*rgb, a)
        for rgb, a in zip(albedo.getdata(), mask.getdata())
    ])

    # Save
    rgba.save(output_path, 'PNG')
    print(f"Created RGBA texture: {output_path}")
    print(f"  Size: {rgba.size}")
    print(f"  Mode: {rgba.mode}")

    return rgba

if __name__ == '__main__':
    import os

    # Paths
    hero_dir = os.path.expanduser('~/Documents/3_Development/Repos/materialize/dist/hero_knobs/series_1')
    albedo_path = os.path.join(hero_dir, 'albedo.png')
    mask_path = os.path.join(hero_dir, 'primitive_1_mask.png')
    output_path = 'assets/ui/hero_knob/albedo_rgba.png'

    # Create RGBA texture
    create_rgba_knob(albedo_path, mask_path, output_path)
    print(f"\nNext: Update CMakeLists.txt to use albedo_rgba.png instead of albedo.png")
