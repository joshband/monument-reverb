#!/usr/bin/env python3
"""Generate complete PBR knob assets with all material layers.

Creates production-ready knob assets with full PBR stack:
- albedo.png (base color/diffuse)
- ao.png (ambient occlusion)
- roughness.png (surface roughness)
- normal.png (surface detail/bumps)
- glow_core.png (center LED glow)
- glow_crystal.png (crystal overlay glow)
- bloom.png (bloom contribution)
- light_wrap.png (edge lighting)
- highlight.png (specular highlights)
- indicator.png (rotation pointer)
- contact_shadow.png (ground shadow)

Requirements:
    pip3 install openai pillow numpy opencv-python scipy

Usage:
    export OPENAI_API_KEY="your-key"
    python3 tools/generate_pbr_knobs.py --knob-type geode --output knob_geode/
"""

import argparse
import os
import sys
from pathlib import Path
import time
from typing import Tuple, Optional

try:
    from openai import OpenAI
    import requests
except ImportError:
    print("❌ Missing openai. Install: pip3 install openai requests")
    sys.exit(1)

try:
    from PIL import Image, ImageFilter, ImageEnhance, ImageDraw, ImageChops
    import numpy as np
except ImportError:
    print("❌ Missing PIL/numpy. Install: pip3 install Pillow numpy")
    sys.exit(1)

try:
    import cv2
except ImportError:
    print("❌ Missing opencv. Install: pip3 install opencv-python")
    sys.exit(1)


# === PROMPTS FOR EACH KNOB TYPE ===

KNOB_PROMPTS = {
    "geode": {
        "albedo": """Photorealistic circular control knob carved from a dark geode crystal,
perfectly centered, orthographic top view, 1024x1024px.

The knob is 40mm diameter, cylindrical with subtle dome.
Surface shows natural crystal structure - dark outer shell transitioning to
deep blue crystalline interior formation at center (like agate or geode cross-section).

Fine mineral details visible: quartz points, druzy texture in center,
polished outer edge with micro-facets, subtle iridescence.

NO glowing, NO emissive elements, NO lighting effects in texture itself.
This is ALBEDO ONLY - pure material color without lighting.

Studio neutral lighting, diffuse illumination only.
Neutral gray background (RGB 128,128,128).
Material-accurate colors, ultra high detail macro photography.
Clean alpha channel for circular knob shape.""",

        "style": "natural",  # More realistic for PBR albedo
    },

    "obsidian": {
        "albedo": """Photorealistic circular control knob carved from polished black obsidian,
perfectly centered, orthographic top view, 1024x1024px.

The knob is 40mm diameter, cylindrical with gentle dome.
Surface is glossy black volcanic glass with subtle gray flow banding,
tiny reflective inclusions (perlite), microscopic surface texture.

NO glowing, NO lighting effects, NO reflections baked into texture.
This is ALBEDO ONLY - base material color (dark gray to black).

Studio neutral diffuse lighting.
Neutral gray background (RGB 128,128,128).
Material-accurate, ultra high detail.
Clean alpha channel.""",

        "style": "natural",
    },

    "marble": {
        "albedo": """Photorealistic circular control knob carved from pale marble,
perfectly centered, orthographic top view, 1024x1024px.

The knob is 40mm diameter, cylindrical with subtle dome.
Surface shows natural marble veining - cream base color with
gray and gold mineral veins flowing organically.

Fine crystalline texture, polished but not mirror finish,
subtle surface variations, geological authenticity.

NO glowing, NO lighting effects.
This is ALBEDO ONLY - pure material color.

Studio neutral diffuse lighting.
Neutral gray background (RGB 128,128,128).
Material-accurate, ultra high detail.
Clean alpha channel.""",

        "style": "natural",
    },

    "weathered_stone": {
        "albedo": """Photorealistic circular control knob carved from ancient weathered basalt,
perfectly centered, orthographic top view, 1024x1024px.

The knob is 40mm diameter, cylindrical with worn dome.
Surface shows age - fine pitting, micro-cracks, erosion patterns,
dark gray with subtle color variations from weathering.

Matte finish, rough texture, archaeological feel,
no polish, natural stone character.

NO glowing, NO lighting effects.
This is ALBEDO ONLY - base material color.

Studio neutral diffuse lighting.
Neutral gray background (RGB 128,128,128).
Material-accurate, ultra high detail.
Clean alpha channel.""",

        "style": "natural",
    },
}


# === GENERATION FUNCTIONS ===

def generate_albedo_dalle3(
    client: OpenAI,
    knob_type: str,
    output_path: Path
) -> bool:
    """Generate albedo texture using DALL-E 3.

    Args:
        client: OpenAI client
        knob_type: Type of knob (geode, obsidian, marble, weathered_stone)
        output_path: Path to save albedo.png

    Returns:
        True if successful
    """
    if knob_type not in KNOB_PROMPTS:
        print(f"❌ Unknown knob type: {knob_type}")
        print(f"   Available: {', '.join(KNOB_PROMPTS.keys())}")
        return False

    prompt_data = KNOB_PROMPTS[knob_type]
    prompt = prompt_data["albedo"]
    style = prompt_data.get("style", "natural")

    print(f"\n🎨 Generating ALBEDO for {knob_type}...")
    print(f"   Prompt: {prompt[:80]}...")
    print(f"   Style: {style}")

    try:
        response = client.images.generate(
            model="dall-e-3",
            prompt=prompt,
            size="1024x1024",
            quality="hd",
            style=style,
            n=1
        )

        image_url = response.data[0].url
        print(f"   ✓ Generated, downloading...")

        # Download
        img_response = requests.get(image_url, timeout=30)
        img_response.raise_for_status()

        # Save
        with open(output_path, 'wb') as f:
            f.write(img_response.content)

        # Post-process to ensure RGBA
        img = Image.open(output_path)
        if img.mode != 'RGBA':
            img = img.convert('RGBA')
        img.save(output_path, "PNG")

        print(f"   ✓ Saved: {output_path}")
        return True

    except Exception as e:
        print(f"   ❌ Failed: {e}")
        return False


def create_ao_from_albedo(albedo_path: Path, output_path: Path):
    """Generate ambient occlusion map from albedo.

    AO represents how exposed each point is to ambient lighting.
    We approximate this by analyzing the luminance and local contrast.

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save ao.png
    """
    print("\n🔧 Generating AMBIENT OCCLUSION...")

    img = Image.open(albedo_path).convert('RGBA')
    gray = img.convert('L')

    # Invert luminance (darker areas = more occluded)
    ao = ImageChops.invert(gray)

    # Enhance contrast
    enhancer = ImageEnhance.Contrast(ao)
    ao = enhancer.enhance(1.5)

    # Slight blur for softer AO
    ao = ao.filter(ImageFilter.GaussianBlur(2))

    # Ensure range 0-255
    ao_array = np.array(ao)
    ao_array = np.clip(ao_array, 0, 255).astype(np.uint8)

    # Convert to RGBA (grayscale in RGB channels, full alpha)
    ao_rgba = Image.new('RGBA', img.size)
    ao_img = Image.fromarray(ao_array)
    ao_rgba.paste(ao_img, (0, 0))

    # Copy alpha from albedo
    alpha = img.split()[3]
    ao_rgba.putalpha(alpha)

    ao_rgba.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_roughness_from_albedo(albedo_path: Path, output_path: Path):
    """Generate roughness map from albedo.

    Roughness represents surface microsurface variation.
    We approximate this from texture detail/high-frequency content.

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save roughness.png
    """
    print("\n🔧 Generating ROUGHNESS...")

    img = Image.open(albedo_path).convert('RGBA')
    gray = img.convert('L')

    # Compute local variance (texture detail)
    gray_array = np.array(gray, dtype=np.float32)

    # High-pass filter to get texture detail
    blurred = cv2.GaussianBlur(gray_array, (21, 21), 0)
    high_freq = gray_array - blurred

    # Normalize to 0-255
    high_freq = np.abs(high_freq)
    high_freq = (high_freq / high_freq.max() * 255).astype(np.uint8)

    # Base roughness (mid-gray = medium roughness)
    base_roughness = 128

    # Modulate base by texture detail
    roughness = base_roughness + (high_freq * 0.3)
    roughness = np.clip(roughness, 0, 255).astype(np.uint8)

    # Convert to RGBA
    roughness_rgba = Image.new('RGBA', img.size)
    roughness_img = Image.fromarray(roughness)
    roughness_rgba.paste(roughness_img, (0, 0))

    # Copy alpha from albedo
    alpha = img.split()[3]
    roughness_rgba.putalpha(alpha)

    roughness_rgba.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_normal_from_albedo(albedo_path: Path, output_path: Path):
    """Generate normal map from albedo using edge detection.

    Normal maps encode surface orientation for lighting calculations.
    We approximate this from luminance gradients.

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save normal.png
    """
    print("\n🔧 Generating NORMAL MAP...")

    img = Image.open(albedo_path).convert('RGBA')
    gray = img.convert('L')
    gray_array = np.array(gray, dtype=np.float32)

    # Compute gradients (Sobel)
    grad_x = cv2.Sobel(gray_array, cv2.CV_64F, 1, 0, ksize=5)
    grad_y = cv2.Sobel(gray_array, cv2.CV_64F, 0, 1, ksize=5)

    # Normalize gradients
    grad_x = grad_x / 255.0
    grad_y = grad_y / 255.0

    # Compute normal vectors
    # Normal = (-dx, -dy, 1) normalized
    normal_z = np.ones_like(grad_x)

    # Normalize vectors
    magnitude = np.sqrt(grad_x**2 + grad_y**2 + normal_z**2)
    normal_x = -grad_x / magnitude
    normal_y = -grad_y / magnitude
    normal_z = normal_z / magnitude

    # Convert to RGB (map [-1,1] to [0,255])
    normal_r = ((normal_x + 1) * 0.5 * 255).astype(np.uint8)
    normal_g = ((normal_y + 1) * 0.5 * 255).astype(np.uint8)
    normal_b = ((normal_z + 1) * 0.5 * 255).astype(np.uint8)

    # Create RGBA normal map
    normal_rgba = Image.merge('RGBA', [
        Image.fromarray(normal_r),
        Image.fromarray(normal_g),
        Image.fromarray(normal_b),
        img.split()[3]  # Copy alpha from albedo
    ])

    normal_rgba.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_glow_core(output_path: Path, size=(1024, 1024)):
    """Generate center LED glow (blue/cyan radial gradient).

    Args:
        output_path: Path to save glow_core.png
        size: Image size
    """
    print("\n✨ Generating GLOW CORE...")

    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = (size[0] // 2, size[1] // 2)
    max_radius = 150

    # Radial gradient (blue-cyan glow)
    for r in range(max_radius, 0, -1):
        alpha = int(255 * (1 - (r / max_radius)) ** 2)
        color = (40, 120, 255, alpha)  # Blue-cyan
        draw.ellipse(
            [center[0] - r, center[1] - r, center[0] + r, center[1] + r],
            fill=color
        )

    img = img.filter(ImageFilter.GaussianBlur(15))
    img.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_glow_crystal(albedo_path: Path, output_path: Path):
    """Generate crystal glow overlay (based on albedo bright areas).

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save glow_crystal.png
    """
    print("\n✨ Generating GLOW CRYSTAL...")

    img = Image.open(albedo_path).convert('RGBA')
    gray = img.convert('L')

    # Extract bright areas (crystalline regions)
    threshold = 100
    gray_array = np.array(gray)
    bright_mask = (gray_array > threshold).astype(np.uint8) * 255

    # Create glow (blue-tinted)
    glow = Image.new('RGBA', img.size, (0, 0, 0, 0))
    glow_array = np.zeros((*img.size[::-1], 4), dtype=np.uint8)

    # Blue glow on bright areas
    glow_array[:, :, 0] = bright_mask // 4  # Red (low)
    glow_array[:, :, 1] = bright_mask // 2  # Green (medium)
    glow_array[:, :, 2] = bright_mask       # Blue (high)
    glow_array[:, :, 3] = bright_mask // 2  # Alpha

    glow = Image.fromarray(glow_array, 'RGBA')
    glow = glow.filter(ImageFilter.GaussianBlur(10))

    glow.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_bloom(output_path: Path, size=(1024, 1024)):
    """Generate bloom contribution map (subtle radial glow).

    Args:
        output_path: Path to save bloom.png
        size: Image size
    """
    print("\n✨ Generating BLOOM...")

    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = (size[0] // 2, size[1] // 2)
    max_radius = 300

    # Soft radial gradient for bloom
    for r in range(max_radius, 0, -5):
        alpha = int(30 * (1 - (r / max_radius)) ** 3)
        color = (200, 220, 255, alpha)  # Soft white-blue
        draw.ellipse(
            [center[0] - r, center[1] - r, center[0] + r, center[1] + r],
            fill=color
        )

    img = img.filter(ImageFilter.GaussianBlur(25))
    img.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_light_wrap(albedo_path: Path, output_path: Path):
    """Generate edge light wrap (rim lighting).

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save light_wrap.png
    """
    print("\n💡 Generating LIGHT WRAP...")

    img = Image.open(albedo_path).convert('RGBA')
    alpha = img.split()[3]
    alpha_array = np.array(alpha)

    # Edge detection on alpha channel
    edges = cv2.Canny(alpha_array, 50, 150)

    # Dilate edges
    kernel = np.ones((5, 5), np.uint8)
    edges = cv2.dilate(edges, kernel, iterations=2)

    # Blur for soft edge light
    edges_blur = cv2.GaussianBlur(edges, (21, 21), 0)

    # Create rim light (warm white)
    light_wrap = Image.new('RGBA', img.size, (0, 0, 0, 0))
    light_array = np.zeros((*img.size[::-1], 4), dtype=np.uint8)

    light_array[:, :, 0] = (edges_blur * 1.0).astype(np.uint8)  # Red
    light_array[:, :, 1] = (edges_blur * 0.95).astype(np.uint8)  # Green
    light_array[:, :, 2] = (edges_blur * 0.8).astype(np.uint8)  # Blue
    light_array[:, :, 3] = (edges_blur * 0.6).astype(np.uint8)  # Alpha

    light_wrap = Image.fromarray(light_array, 'RGBA')
    light_wrap.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_highlight(albedo_path: Path, output_path: Path):
    """Generate specular highlight map.

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save highlight.png
    """
    print("\n💡 Generating HIGHLIGHT...")

    img = Image.open(albedo_path).convert('RGBA')
    gray = img.convert('L')
    gray_array = np.array(gray, dtype=np.float32)

    # Create specular highlight (brightest areas)
    highlight = np.power(gray_array / 255.0, 4) * 255
    highlight = highlight.astype(np.uint8)

    # Blur slightly
    highlight = cv2.GaussianBlur(highlight, (7, 7), 0)

    # Create RGBA
    highlight_rgba = Image.new('RGBA', img.size, (0, 0, 0, 0))
    highlight_img = Image.fromarray(highlight)

    # White highlights with alpha
    highlight_rgba = Image.merge('RGBA', [
        highlight_img,
        highlight_img,
        highlight_img,
        Image.fromarray((highlight * 0.7).astype(np.uint8))
    ])

    highlight_rgba.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_indicator(output_path: Path, size=(1024, 1024)):
    """Generate rotation indicator (line pointer).

    Args:
        output_path: Path to save indicator.png
        size: Image size
    """
    print("\n📍 Generating INDICATOR...")

    img = Image.new('RGBA', size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    center = (size[0] // 2, size[1] // 2)

    # Vertical line from center upward
    line_width = 6
    line_length = 180

    start_y = center[1] - 30
    end_y = center[1] - line_length

    draw.rectangle(
        [center[0] - line_width // 2, end_y, center[0] + line_width // 2, start_y],
        fill=(255, 255, 255, 255)
    )

    # Slight blur for antialiasing
    img = img.filter(ImageFilter.GaussianBlur(1))

    img.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def create_contact_shadow(albedo_path: Path, output_path: Path):
    """Generate contact shadow (ground plane shadow).

    Args:
        albedo_path: Path to albedo.png
        output_path: Path to save contact_shadow.png
    """
    print("\n🌑 Generating CONTACT SHADOW...")

    img = Image.open(albedo_path).convert('RGBA')
    alpha = img.split()[3]
    alpha_array = np.array(alpha)

    # Create soft shadow around knob base
    shadow = Image.new('RGBA', img.size, (0, 0, 0, 0))
    shadow_array = np.zeros((*img.size[::-1], 4), dtype=np.uint8)

    # Dilate alpha to create shadow region
    kernel = np.ones((15, 15), np.uint8)
    shadow_mask = cv2.dilate(alpha_array, kernel, iterations=3)

    # Blur heavily for soft shadow
    shadow_mask = cv2.GaussianBlur(shadow_mask, (41, 41), 0)

    # Dark shadow
    shadow_array[:, :, 0] = 0
    shadow_array[:, :, 1] = 0
    shadow_array[:, :, 2] = 0
    shadow_array[:, :, 3] = (shadow_mask * 0.4).astype(np.uint8)

    shadow = Image.fromarray(shadow_array, 'RGBA')
    shadow.save(output_path, "PNG")
    print(f"   ✓ Saved: {output_path}")


def generate_complete_knob(
    knob_type: str,
    output_dir: Path,
    api_key: Optional[str] = None
):
    """Generate complete PBR knob with all layers.

    Args:
        knob_type: Type of knob (geode, obsidian, marble, weathered_stone)
        output_dir: Output directory
        api_key: OpenAI API key (or from env)
    """
    output_dir.mkdir(parents=True, exist_ok=True)

    print(f"\n{'='*60}")
    print(f"🎨 GENERATING COMPLETE PBR KNOB: {knob_type}")
    print(f"{'='*60}")

    # Initialize OpenAI client
    api_key = api_key or os.environ.get("OPENAI_API_KEY")
    if not api_key:
        print("❌ No OpenAI API key found. Set OPENAI_API_KEY env var.")
        sys.exit(1)

    client = OpenAI(api_key=api_key)

    # 1. Generate albedo (DALL-E 3)
    albedo_path = output_dir / "albedo.png"
    if not generate_albedo_dalle3(client, knob_type, albedo_path):
        print("❌ Failed to generate albedo")
        return

    # Wait a bit to avoid rate limits
    time.sleep(2)

    # 2. Generate derived maps from albedo
    create_ao_from_albedo(albedo_path, output_dir / "ao.png")
    create_roughness_from_albedo(albedo_path, output_dir / "roughness.png")
    create_normal_from_albedo(albedo_path, output_dir / "normal.png")

    # 3. Generate glow layers
    create_glow_core(output_dir / "glow_core.png")
    create_glow_crystal(albedo_path, output_dir / "glow_crystal.png")
    create_bloom(output_dir / "bloom.png")

    # 4. Generate lighting layers
    create_light_wrap(albedo_path, output_dir / "light_wrap.png")
    create_highlight(albedo_path, output_dir / "highlight.png")

    # 5. Generate indicator and shadow
    create_indicator(output_dir / "indicator.png")
    create_contact_shadow(albedo_path, output_dir / "contact_shadow.png")

    print(f"\n{'='*60}")
    print(f"✅ COMPLETE! All layers generated in: {output_dir}")
    print(f"{'='*60}")
    print("\nGenerated layers:")
    for layer in sorted(output_dir.glob("*.png")):
        size_kb = layer.stat().st_size / 1024
        print(f"  ✓ {layer.name:<25} ({size_kb:>6.1f} KB)")


def main():
    parser = argparse.ArgumentParser(
        description="Generate complete PBR knob assets with all material layers"
    )
    parser.add_argument(
        "--knob-type",
        type=str,
        required=True,
        choices=["geode", "obsidian", "marble", "weathered_stone"],
        help="Type of knob to generate"
    )
    parser.add_argument(
        "--output",
        type=Path,
        required=True,
        help="Output directory (e.g., knob_geode/)"
    )
    parser.add_argument(
        "--api-key",
        type=str,
        help="OpenAI API key (or use OPENAI_API_KEY env var)"
    )

    args = parser.parse_args()

    generate_complete_knob(
        knob_type=args.knob_type,
        output_dir=args.output,
        api_key=args.api_key
    )


if __name__ == "__main__":
    main()
