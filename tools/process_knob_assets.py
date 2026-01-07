#!/usr/bin/env python3
"""Process existing AI-generated knob assets for Monument UI Demo.

Takes raw Midjourney/DALL-E outputs from UI Mockup/images/knobs/
and processes them into proper layered RGBA assets for JUCE.

Features:
- Background removal (white → transparent)
- Center crop and resize to 1024×1024 or 512×512
- Layer separation (stone base vs glow overlay)
- Proper alpha channel creation
- Organized output into stone/crystal/core folders

Usage:
    python3 tools/process_knob_assets.py --input "UI Mockup/images/knobs" --output "MonumentUI_Demo/Assets/knobs"
"""

import argparse
import sys
from pathlib import Path
from typing import Tuple

try:
    from PIL import Image, ImageOps, ImageChops
    import numpy as np
except ImportError:
    print("❌ Missing dependencies. Install with:")
    print("   pip3 install Pillow numpy")
    sys.exit(1)


def remove_background(img: Image.Image, threshold: int = 240) -> Image.Image:
    """Remove white background and create alpha channel.

    Args:
        img: Input image (RGB or RGBA)
        threshold: Brightness threshold for background (0-255)

    Returns:
        RGBA image with transparent background
    """
    # Convert to RGBA if needed
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Get image data as numpy array
    data = np.array(img)

    # Create mask: pixels where R, G, B all > threshold
    r, g, b, a = data[:,:,0], data[:,:,1], data[:,:,2], data[:,:,3]
    white_mask = (r > threshold) & (g > threshold) & (b > threshold)

    # Set alpha to 0 for white pixels
    data[:,:,3] = np.where(white_mask, 0, a)

    return Image.fromarray(data, 'RGBA')


def center_crop_square(img: Image.Image) -> Image.Image:
    """Center crop to largest square.

    Args:
        img: Input image

    Returns:
        Square cropped image
    """
    width, height = img.size
    size = min(width, height)

    left = (width - size) // 2
    top = (height - size) // 2
    right = left + size
    bottom = top + size

    return img.crop((left, top, right, bottom))


def auto_trim(img: Image.Image, border: int = 10) -> Image.Image:
    """Trim transparent borders with optional padding.

    Args:
        img: RGBA image
        border: Padding to keep around content (pixels)

    Returns:
        Trimmed image
    """
    # Get bounding box of non-transparent pixels
    bbox = img.getbbox()

    if bbox:
        # Add border padding
        left = max(0, bbox[0] - border)
        top = max(0, bbox[1] - border)
        right = min(img.width, bbox[2] + border)
        bottom = min(img.height, bbox[3] + border)

        return img.crop((left, top, right, bottom))

    return img


def separate_glow_layer(img: Image.Image, glow_threshold: int = 100) -> Tuple[Image.Image, Image.Image]:
    """Separate base and glow layers.

    Attempts to detect bright glowing regions and extract them to a separate layer.

    Args:
        img: RGBA image with potential glow
        glow_threshold: Brightness threshold for glow detection

    Returns:
        (base_layer, glow_layer) tuple of RGBA images
    """
    data = np.array(img)
    r, g, b, a = data[:,:,0], data[:,:,1], data[:,:,2], data[:,:,3]

    # Calculate brightness
    brightness = (r.astype(float) + g.astype(float) + b.astype(float)) / 3.0

    # Create glow mask (bright regions)
    glow_mask = brightness > glow_threshold

    # Create base layer (dim glow in bright areas)
    base_data = data.copy()
    base_data[:,:,3] = np.where(glow_mask, a * 0.3, a).astype(np.uint8)

    # Create glow layer (only bright areas)
    glow_data = data.copy()
    glow_data[:,:,3] = np.where(glow_mask, a, 0).astype(np.uint8)

    base_layer = Image.fromarray(base_data, 'RGBA')
    glow_layer = Image.fromarray(glow_data, 'RGBA')

    return base_layer, glow_layer


def process_stone_knob(input_path: Path, output_dir: Path, variant: int):
    """Process a stone knob image.

    Args:
        input_path: Path to source image
        output_dir: Output directory for processed assets
        variant: Variant number (1-4)
    """
    print(f"  Processing stone knob variant {variant}: {input_path.name}")

    img = Image.open(input_path)

    # 1. Remove white background
    img = remove_background(img, threshold=240)

    # 2. Center crop to square
    img = center_crop_square(img)

    # 3. Auto-trim with padding
    img = auto_trim(img, border=20)

    # 4. Resize to 1024×1024
    img = img.resize((1024, 1024), Image.Resampling.LANCZOS)

    # 5. Save
    output_path = output_dir / "stone" / f"knob_stone_{variant:02d}.png"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(output_path, "PNG")

    print(f"    ✓ Saved: {output_path.relative_to(output_dir.parent.parent)}")


def process_crystal_glow(input_path: Path, output_dir: Path, color: str):
    """Process a crystal glow image.

    Args:
        input_path: Path to source image
        output_dir: Output directory for processed assets
        color: Color name (blue, gold, white, etc.)
    """
    print(f"  Processing crystal glow ({color}): {input_path.name}")

    img = Image.open(input_path)

    # 1. Remove background
    img = remove_background(img, threshold=230)

    # 2. Center crop
    img = center_crop_square(img)

    # 3. Auto-trim
    img = auto_trim(img, border=50)

    # 4. Resize to 1024×1024
    img = img.resize((1024, 1024), Image.Resampling.LANCZOS)

    # 5. Enhance alpha channel (ensure proper gradient falloff)
    data = np.array(img)
    alpha = data[:,:,3]

    # Soft feather the edges
    from scipy.ndimage import gaussian_filter
    alpha_smooth = gaussian_filter(alpha.astype(float), sigma=3)
    data[:,:,3] = alpha_smooth.astype(np.uint8)

    img = Image.fromarray(data, 'RGBA')

    # 6. Save
    output_path = output_dir / "crystal" / f"crystal_glow_{color}.png"
    output_path.parent.mkdir(parents=True, exist_ok=True)
    img.save(output_path, "PNG")

    print(f"    ✓ Saved: {output_path.relative_to(output_dir.parent.parent)}")


def analyze_assets(input_dir: Path):
    """Analyze existing assets and suggest processing strategy.

    Args:
        input_dir: Directory containing raw assets
    """
    print(f"\n📊 Analyzing assets in: {input_dir}\n")

    all_files = sorted(input_dir.glob("*.png"))

    stone_knobs = [f for f in all_files if "stone_rotary_knob" in f.name]
    led_glows = [f for f in all_files if "amorphous_warm_glowing" in f.name or "glowing_led" in f.name]
    switches = [f for f in all_files if "switch_toggle" in f.name]
    pbr_maps = [f for f in all_files if f.name in ["Albedo.png", "AORoughness.png", "EnvironmentalGlow.png", "RGB Image.png", "RGBA-Image.png"]]

    print(f"Stone knobs: {len(stone_knobs)} files")
    for f in stone_knobs[:3]:
        print(f"  - {f.name}")
    if len(stone_knobs) > 3:
        print(f"  ... and {len(stone_knobs) - 3} more")

    print(f"\nLED/Crystal glows: {len(led_glows)} files")
    for f in led_glows[:3]:
        print(f"  - {f.name}")
    if len(led_glows) > 3:
        print(f"  ... and {len(led_glows) - 3} more")

    print(f"\nStone switches: {len(switches)} files")
    print(f"PBR map examples: {len(pbr_maps)} files")

    print(f"\n✅ You have enough assets to build the layered knob system!")
    print(f"\nRecommended processing:")
    print(f"  1. Select best 4 stone knob variants → stone/ folder")
    print(f"  2. Select 3-4 LED glow variants → crystal/ folder")
    print(f"  3. Generate or source metal cores separately → core/ folder")
    print(f"  4. Create simple indicator graphics → indicator/ folder")


def main():
    parser = argparse.ArgumentParser(
        description="Process AI-generated knob assets for Monument UI",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--input",
        type=str,
        default="UI Mockup/images/knobs",
        help="Input directory with raw assets"
    )

    parser.add_argument(
        "--output",
        type=str,
        default="MonumentUI_Demo/Assets/knobs",
        help="Output directory for processed assets"
    )

    parser.add_argument(
        "--analyze-only",
        action="store_true",
        help="Only analyze assets, don't process"
    )

    parser.add_argument(
        "--stone-variants",
        type=str,
        nargs='+',
        help="Specific stone knob files to process (by number or UUID)"
    )

    parser.add_argument(
        "--glow-variants",
        type=str,
        nargs='+',
        help="Specific glow files to process"
    )

    args = parser.parse_args()

    # Resolve paths
    input_dir = Path(args.input)
    output_dir = Path(args.output)

    if not input_dir.exists():
        print(f"❌ Input directory not found: {input_dir}")
        sys.exit(1)

    # Analyze assets
    analyze_assets(input_dir)

    if args.analyze_only:
        return

    # Process assets
    print(f"\n🔄 Processing assets...\n")

    # Get all stone knob files
    stone_files = sorted(input_dir.glob("stone_rotary_knob*.png"))

    if args.stone_variants:
        # Filter based on user selection
        stone_files = [f for f in stone_files if any(v in f.name for v in args.stone_variants)]

    # Select up to 4 unique variants (group by UUID)
    stone_variants = {}
    for f in stone_files:
        uuid = f.stem.split('_')[-2]  # Extract UUID
        if uuid not in stone_variants:
            stone_variants[uuid] = f
            if len(stone_variants) >= 4:
                break

    # Process stone knobs
    for i, (uuid, path) in enumerate(sorted(stone_variants.items()), start=1):
        process_stone_knob(path, output_dir, variant=i)

    # Get all glow files
    glow_files = sorted(input_dir.glob("amorphous_warm_glowing*.png"))

    if args.glow_variants:
        glow_files = [f for f in glow_files if any(v in f.name for v in args.glow_variants)]

    # Select up to 3 unique variants
    glow_variants = {}
    for f in glow_files:
        uuid = f.stem.split('_')[-2]
        if uuid not in glow_variants:
            glow_variants[uuid] = f
            if len(glow_variants) >= 3:
                break

    # Process glows (name them by warmth/color)
    glow_colors = ["warm", "gold", "amber"]
    for (uuid, path), color in zip(sorted(glow_variants.items()), glow_colors):
        process_crystal_glow(path, output_dir, color=color)

    print(f"\n✅ Processing complete!")
    print(f"\nNext steps:")
    print(f"  1. Review processed assets in: {output_dir}")
    print(f"  2. Generate metal core caps (see UI Mockup/knobPrompts.md)")
    print(f"  3. Create simple indicator graphics")
    print(f"  4. Update MonumentUI_Demo/CMakeLists.txt BinaryData section")
    print(f"  5. Rebuild: cmake --build build -j8")


if __name__ == "__main__":
    main()
