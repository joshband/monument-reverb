#!/usr/bin/env python3
"""
Pack Monument hero knob PBR maps into optimized textures for JUCE rendering.

Creates:
1. albedo_rgba.png - Albedo with circular alpha mask (removes white background)
2. packed_rmao.png - R=roughness, G=metallic, B=ambient occlusion, A=255 (unused)
"""

from PIL import Image, ImageDraw
import numpy as np
import os

def create_circular_mask(size):
    """Create a circular alpha mask with soft edge."""
    mask = Image.new('L', (size, size), 0)
    draw = ImageDraw.Draw(mask)

    # Draw circle filling 90% of the frame (matching the knob)
    margin = int(size * 0.05)  # 5% margin on each side
    draw.ellipse([margin, margin, size - margin, size - margin], fill=255)

    # Apply slight gaussian blur for soft edge
    from PIL import ImageFilter
    mask = mask.filter(ImageFilter.GaussianBlur(radius=2))

    return mask

def remove_white_background(image_path, output_path):
    """Remove white background and add circular alpha mask."""
    print(f"Processing albedo with alpha mask...")

    # Load image
    img = Image.open(image_path).convert('RGB')
    width, height = img.size

    # Create circular mask
    mask = create_circular_mask(width)

    # Convert to numpy for color-based masking
    img_array = np.array(img)

    # Identify near-white pixels (background)
    # White threshold: RGB values all > 240
    white_mask = np.all(img_array > 240, axis=2)

    # Convert mask to PIL
    white_mask_pil = Image.fromarray((~white_mask * 255).astype(np.uint8))

    # Combine circular mask with white removal
    final_mask = Image.composite(mask, Image.new('L', img.size, 0), white_mask_pil)

    # Create RGBA image
    rgba = Image.new('RGBA', img.size)
    rgba.putdata([
        (*rgb, a)
        for rgb, a in zip(img.getdata(), final_mask.getdata())
    ])

    rgba.save(output_path, 'PNG')
    print(f"  ✓ Created: {output_path}")
    return rgba

def pack_pbr_maps(roughness_path, metallic_path, ao_path, output_path):
    """Pack R=roughness, G=metallic, B=AO into single RGB texture."""
    print(f"Packing PBR maps (R=roughness, G=metallic, B=AO)...")

    # Load maps as grayscale
    roughness = Image.open(roughness_path).convert('L')
    metallic = Image.open(metallic_path).convert('L').resize(roughness.size, Image.LANCZOS)
    ao = Image.open(ao_path).convert('L').resize(roughness.size, Image.LANCZOS)

    # Create fully opaque alpha channel (unused but good for compatibility)
    alpha = Image.new('L', roughness.size, 255)

    # Merge into RGBA
    packed = Image.merge('RGBA', (roughness, metallic, ao, alpha))
    packed.save(output_path, 'PNG')
    print(f"  ✓ Created: {output_path}")
    return packed

def main():
    """Process and pack all PBR maps."""
    input_dir = "assets/ui/hero_knob_pbr"
    output_dir = "assets/ui/hero_knob_pbr"

    # Check input files exist
    required_files = ['albedo.png', 'roughness.png', 'metallic.png', 'ao.png']
    for f in required_files:
        path = os.path.join(input_dir, f)
        if not os.path.exists(path):
            print(f"ERROR: Missing {path}")
            return False

    # Process albedo with alpha
    albedo_rgba = remove_white_background(
        os.path.join(input_dir, 'albedo.png'),
        os.path.join(output_dir, 'albedo_rgba.png')
    )

    # Pack PBR maps
    packed_rmao = pack_pbr_maps(
        os.path.join(input_dir, 'roughness.png'),
        os.path.join(input_dir, 'metallic.png'),
        os.path.join(input_dir, 'ao.png'),
        os.path.join(output_dir, 'packed_rmao.png')
    )

    print(f"\n✅ PBR textures packed successfully!")
    print(f"\nGenerated files:")
    print(f"  • albedo_rgba.png  - Base color with alpha mask (for rendering)")
    print(f"  • packed_rmao.png  - Roughness/Metallic/AO packed (for future PBR)")
    print(f"  • normal.png       - Normal map (preserved for future use)")

    print(f"\nNext steps:")
    print(f"1. Check albedo_rgba.png for correct alpha masking")
    print(f"2. Update CMakeLists.txt to use albedo_rgba.png")
    print(f"3. Rebuild plugin: cmake --build build --target Monument_Standalone --clean-first")

    return True

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
