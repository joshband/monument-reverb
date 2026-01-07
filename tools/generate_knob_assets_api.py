#!/usr/bin/env python3
"""Generate knob assets via OpenAI DALL-E 3 API.

Reads prompts from UI Mockup/knobPrompts.md and generates missing assets
using OpenAI's DALL-E 3 image generation API.

Requirements:
    pip3 install openai pillow

Usage:
    export OPENAI_API_KEY="your-api-key-here"
    python3 tools/generate_knob_assets_api.py --asset-type metal_cores --count 3
    python3 tools/generate_knob_assets_api.py --asset-type indicators --count 2
    python3 tools/generate_knob_assets_api.py --asset-type stone_knobs --variants 2,3
"""

import argparse
import os
import sys
import time
from pathlib import Path
from typing import List, Dict

try:
    from openai import OpenAI
except ImportError:
    print("❌ Missing openai package. Install with: pip3 install openai")
    sys.exit(1)

try:
    from PIL import Image
    import requests
except ImportError:
    print("❌ Missing dependencies. Install with: pip3 install Pillow requests")
    sys.exit(1)


# === PROMPT TEMPLATES ===

PROMPTS = {
    "stone_knob_01": """A photorealistic circular control knob carved from polished dark granite,
perfectly centered, viewed from directly above (orthographic).

The knob is cylindrical with a subtle dome top,
diameter approximately 40mm, height visible from side profile.

Surface shows fine mineral flecks (quartz, mica, feldspar),
subtle blue-gray veining beneath the surface,
polished to a satin finish with micro-scratches from use.

Edges are slightly beveled with gentle wear,
no sharp corners, feels hand-machined.

Lighting is soft studio setup from 45° above,
minimal shadow, controlled highlights only,
no rim lighting, no dramatic effects.

photorealistic, material-accurate, industrial macro photography,
neutral gray background (RGB 128,128,128),
orthographic camera, perfectly centered, ultra high detail""",

    "stone_knob_02": """A photorealistic circular control knob cut from dark weathered basalt,
perfectly centered, orthographic top view.

The knob shows more surface texture than polished granite:
fine pitting, microscopic cracks, geological age visible.
Surface is matte with subtle variations in darkness.

Edges are slightly worn, not perfectly circular,
giving an excavated/archaeological feeling.

photorealistic, material-accurate, industrial macro photography,
neutral gray background, orthographic camera, perfectly centered""",

    "metal_core_brushed": """A photorealistic circular metal cap for a knob center,
brushed aluminum with linear grain pattern,
perfectly centered, orthographic top view.

Diameter approximately 15mm (smaller than full knob),
slight dome profile, edges chamfered smoothly.

Surface shows fine circular brushing marks radiating from center,
creating anisotropic reflections.

Color: neutral aluminum (RGB ~180, 180, 185),
satin finish, not mirror polished but not matte.

photorealistic, material-accurate, industrial macro photography,
neutral gray background, orthographic camera, perfectly centered""",

    "metal_core_brass": """A photorealistic circular metal cap for a knob center,
polished brass with warm golden color,
perfectly centered, orthographic top view.

Diameter approximately 15mm,
slight dome with smooth chamfered edges.

Surface is highly polished with subtle wear,
shows specular reflections and minor aging patina.

Color: warm brass (RGB ~195, 165, 105),
high gloss but not mirror-perfect.

photorealistic, material-accurate, industrial macro photography,
neutral gray background, orthographic camera, perfectly centered""",

    "metal_core_copper": """A photorealistic circular metal cap for a knob center,
copper with subtle green-blue patina forming,
perfectly centered, orthographic top view.

Diameter approximately 15mm,
dome profile with chamfered edges.

Surface shows aged copper with irregular patina patterns,
mix of copper color (warm) and verdigris (cool green-blue).
Not fully oxidized, partial aging only.

photorealistic, material-accurate, industrial macro photography,
neutral gray background, orthographic camera, perfectly centered""",

    "indicator_line": """A simple linear pointer indicator for a rotary knob,
thin rectangular bar extending from center,
perfectly centered, orthographic top view.

Dimensions: 2-3mm wide, 20mm long,
positioned vertically (pointing to 12 o'clock).

Material: brushed aluminum or white painted metal,
very simple, minimal detail, functional not decorative.

Color: light neutral (RGB ~200, 200, 205) OR pure white.

minimal geometric style, clean form,
neutral gray background, orthographic camera, perfectly centered""",

    "indicator_dot": """A small circular dot marker for a rotary knob position indicator,
simple geometric circle near the edge of knob radius,
perfectly centered composition, orthographic top view.

Dot diameter: 4-5mm,
positioned at approximately 80% of knob radius from center,
at 12 o'clock position.

Color: pure white (RGB 255, 255, 255) OR light gray.

minimal geometric style, clean simple form,
neutral gray background, orthographic camera, perfectly centered""",
}


def generate_image_dalle3(
    client: OpenAI,
    prompt: str,
    size: str = "1024x1024",
    quality: str = "hd",
    style: str = "natural"
) -> str:
    """Generate image using DALL-E 3.

    Args:
        client: OpenAI client
        prompt: Text prompt
        size: Image size (1024x1024, 1024x1792, or 1792x1024)
        quality: Quality (standard or hd)
        style: Style (natural or vivid)

    Returns:
        URL of generated image
    """
    print(f"  Generating with DALL-E 3...")
    print(f"  Size: {size}, Quality: {quality}, Style: {style}")

    response = client.images.generate(
        model="dall-e-3",
        prompt=prompt,
        size=size,
        quality=quality,
        style=style,
        n=1
    )

    image_url = response.data[0].url
    return image_url


def download_image(url: str, output_path: Path):
    """Download image from URL.

    Args:
        url: Image URL
        output_path: Local path to save
    """
    print(f"  Downloading...")
    response = requests.get(url)
    response.raise_for_status()

    with open(output_path, 'wb') as f:
        f.write(response.content)

    print(f"  ✓ Saved: {output_path}")


def post_process_image(input_path: Path, size: tuple = (1024, 1024)):
    """Post-process generated image.

    Args:
        input_path: Path to image
        size: Target size
    """
    img = Image.open(input_path)

    # Ensure RGBA
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Resize if needed
    if img.size != size:
        img = img.resize(size, Image.Resampling.LANCZOS)

    # Save
    img.save(input_path, "PNG")


def generate_asset(
    client: OpenAI,
    asset_type: str,
    variant: int,
    output_dir: Path,
    size: str = "1024x1024",
    quality: str = "hd"
):
    """Generate a single asset.

    Args:
        client: OpenAI client
        asset_type: Asset type key (e.g., "metal_core_brushed")
        variant: Variant number for naming
        output_dir: Output directory
        size: Image size
        quality: Generation quality
    """
    if asset_type not in PROMPTS:
        print(f"❌ Unknown asset type: {asset_type}")
        return

    prompt = PROMPTS[asset_type]

    print(f"\n🎨 Generating: {asset_type} (variant {variant})")
    print(f"Prompt: {prompt[:100]}...")

    try:
        # Generate
        image_url = generate_image_dalle3(client, prompt, size=size, quality=quality)

        # Determine output path
        if "stone_knob" in asset_type:
            filename = f"knob_stone_{variant:02d}_generated.png"
            output_path = output_dir / "stone" / filename
        elif "metal_core" in asset_type:
            material = asset_type.split('_')[-1]
            filename = f"core_metal_{material}_generated.png"
            output_path = output_dir / "core" / filename
        elif "indicator" in asset_type:
            marker_type = asset_type.split('_')[-1]
            filename = f"indicator_{marker_type}_generated.png"
            output_path = output_dir / "indicator" / filename
        else:
            filename = f"{asset_type}_{variant:02d}_generated.png"
            output_path = output_dir / filename

        # Create directory
        output_path.parent.mkdir(parents=True, exist_ok=True)

        # Download
        download_image(image_url, output_path)

        # Post-process
        target_size = (512, 512) if "core" in asset_type or "indicator" in asset_type else (1024, 1024)
        post_process_image(output_path, size=target_size)

        print(f"  ✅ Complete: {output_path.relative_to(output_dir.parent.parent)}")

    except Exception as e:
        print(f"  ❌ Error: {e}")


def main():
    parser = argparse.ArgumentParser(
        description="Generate knob assets via OpenAI DALL-E 3 API",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument(
        "--asset-type",
        type=str,
        required=True,
        choices=[
            "stone_knobs", "metal_cores", "indicators",
            "stone_knob_01", "stone_knob_02",
            "metal_core_brushed", "metal_core_brass", "metal_core_copper",
            "indicator_line", "indicator_dot"
        ],
        help="Asset type to generate"
    )

    parser.add_argument(
        "--count",
        type=int,
        default=1,
        help="Number of variants to generate"
    )

    parser.add_argument(
        "--variants",
        type=str,
        help="Comma-separated variant numbers (e.g., '1,3,4')"
    )

    parser.add_argument(
        "--output",
        type=str,
        default="MonumentUI_Demo/Assets/knobs",
        help="Output directory"
    )

    parser.add_argument(
        "--quality",
        type=str,
        choices=["standard", "hd"],
        default="hd",
        help="Generation quality"
    )

    parser.add_argument(
        "--size",
        type=str,
        choices=["1024x1024", "1024x1792", "1792x1024"],
        default="1024x1024",
        help="Image size"
    )

    parser.add_argument(
        "--delay",
        type=int,
        default=5,
        help="Delay between generations (seconds)"
    )

    args = parser.parse_args()

    # Check for API key
    api_key = os.getenv("OPENAI_API_KEY")
    if not api_key:
        print("❌ OPENAI_API_KEY environment variable not set")
        print("\nSet it with:")
        print("  export OPENAI_API_KEY='your-api-key-here'")
        sys.exit(1)

    # Initialize client
    client = OpenAI(api_key=api_key)
    output_dir = Path(args.output)

    # Determine which assets to generate
    if args.asset_type == "stone_knobs":
        asset_types = ["stone_knob_01", "stone_knob_02"]
    elif args.asset_type == "metal_cores":
        asset_types = ["metal_core_brushed", "metal_core_brass", "metal_core_copper"]
    elif args.asset_type == "indicators":
        asset_types = ["indicator_line", "indicator_dot"]
    else:
        asset_types = [args.asset_type]

    # Determine variants
    if args.variants:
        variants = [int(v.strip()) for v in args.variants.split(',')]
    else:
        variants = list(range(1, args.count + 1))

    print(f"\n🚀 Starting generation...")
    print(f"Asset types: {asset_types}")
    print(f"Variants: {variants}")
    print(f"Output: {output_dir}")
    print(f"Quality: {args.quality}")
    print(f"Size: {args.size}")

    # Generate assets
    total = len(asset_types) * len(variants)
    current = 0

    for asset_type in asset_types:
        for variant in variants:
            current += 1
            print(f"\n[{current}/{total}]")

            generate_asset(
                client,
                asset_type,
                variant,
                output_dir,
                size=args.size,
                quality=args.quality
            )

            # Rate limiting delay
            if current < total:
                print(f"\n⏳ Waiting {args.delay}s before next generation...")
                time.sleep(args.delay)

    print(f"\n✅ Generation complete!")
    print(f"\nGenerated {current} assets in: {output_dir}")
    print(f"\nNext steps:")
    print(f"  1. Review generated assets")
    print(f"  2. Update CMakeLists.txt BinaryData section")
    print(f"  3. Rebuild: cmake --build build -j8")


if __name__ == "__main__":
    main()
