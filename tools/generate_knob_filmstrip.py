#!/usr/bin/env python3
"""
Generate filmstrip from PBR knob layers with proper blend modes.

This script composites all PBR layers (albedo, AO, roughness, normal, glow, etc.)
across 64 rotation positions and outputs a vertical filmstrip for zero-cost runtime rendering.

Usage:
    python3 tools/generate_knob_filmstrip.py [knob_name]

Example:
    python3 tools/generate_knob_filmstrip.py knob_geode
"""

import sys
from pathlib import Path
from PIL import Image, ImageDraw, ImageFilter
import numpy as np

# Project paths
PROJECT_ROOT = Path(__file__).parent.parent
ASSETS_DIR = PROJECT_ROOT / "MonumentUI_Demo" / "Assets" / "knobs"
OUTPUT_DIR = PROJECT_ROOT / "MonumentUI_Demo" / "Assets" / "knobs_filmstrip"


def blend_multiply(base: np.ndarray, blend: np.ndarray, opacity: float = 1.0) -> np.ndarray:
    """Multiply blend mode: base * blend"""
    result = (base / 255.0) * (blend / 255.0) * 255.0
    return (base * (1 - opacity) + result * opacity).astype(np.uint8)


def blend_screen(base: np.ndarray, blend: np.ndarray, opacity: float = 1.0) -> np.ndarray:
    """Screen blend mode: 1 - (1-base) * (1-blend)"""
    base_norm = base / 255.0
    blend_norm = blend / 255.0
    result = (1.0 - (1.0 - base_norm) * (1.0 - blend_norm)) * 255.0
    return (base * (1 - opacity) + result * opacity).astype(np.uint8)


def blend_additive(base: np.ndarray, blend: np.ndarray, opacity: float = 1.0) -> np.ndarray:
    """Additive blend mode: base + blend (clamped)"""
    result = np.clip(base + blend * opacity, 0, 255)
    return result.astype(np.uint8)


def create_circular_alpha_mask(size: int, feather_pixels: int = 4) -> Image.Image:
    """
    Create a circular alpha mask with soft edge.

    Args:
        size: Image dimension (assumes square)
        feather_pixels: Soft edge blur radius

    Returns:
        Grayscale mask image
    """
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)

    center = size // 2
    # Slightly smaller than image to avoid edge artifacts
    radius = int(size * 0.48)

    draw.ellipse([center - radius, center - radius,
                  center + radius, center + radius], fill=255)

    # Soft edge
    if feather_pixels > 0:
        mask = mask.filter(ImageFilter.GaussianBlur(feather_pixels))

    return mask


def rotate_image(img: Image.Image, angle_degrees: float) -> Image.Image:
    """
    Rotate image around center with high quality.

    Args:
        img: Input RGBA image
        angle_degrees: Rotation angle in degrees (positive = clockwise)

    Returns:
        Rotated RGBA image
    """
    return img.rotate(-angle_degrees, resample=Image.BICUBIC, expand=False)


def load_layer(layer_path: Path, apply_mask: bool = False, mask: Image.Image = None) -> Image.Image:
    """Load a PBR layer and optionally apply circular mask."""
    img = Image.open(layer_path).convert("RGBA")

    if apply_mask and mask is not None:
        # Resize mask to match image size if needed
        if mask.size != img.size:
            mask_resized = mask.resize(img.size, Image.LANCZOS)
        else:
            mask_resized = mask

        # Apply mask to alpha channel
        r, g, b, a = img.split()
        # Combine existing alpha with mask
        new_alpha = Image.composite(a, Image.new("L", img.size, 0), mask_resized)
        img.putalpha(new_alpha)

    return img


def composite_frame(layers: dict, rotation_degrees: float, size: int) -> Image.Image:
    """
    Composite all PBR layers for a single knob rotation position.

    Args:
        layers: Dict of layer name -> PIL Image
        rotation_degrees: Knob rotation (0-360)
        size: Output image size

    Returns:
        Composited RGBA frame
    """
    # Create output canvas
    result = Image.new("RGBA", (size, size), (0, 0, 0, 0))

    # Rotate layers that should move with the knob
    rotatable_layers = ['albedo', 'ao', 'roughness', 'normal', 'indicator']
    rotated = {}
    for name in rotatable_layers:
        if name in layers:
            rotated[name] = rotate_image(layers[name], rotation_degrees)

    # Start with albedo as base
    if 'albedo' in rotated:
        result = rotated['albedo'].copy()
        result_array = np.array(result)
    else:
        return result

    # Extract RGB and alpha channels
    rgb = result_array[:, :, :3]
    alpha = result_array[:, :, 3]

    # Apply AO (multiply blend)
    if 'ao' in rotated:
        ao_array = np.array(rotated['ao'])
        rgb = blend_multiply(rgb, ao_array[:, :, :3], opacity=0.7)

    # Apply roughness (multiply blend)
    if 'roughness' in rotated:
        roughness_array = np.array(rotated['roughness'])
        rgb = blend_multiply(rgb, roughness_array[:, :, :3], opacity=0.5)

    # Apply contact shadow (multiply blend, non-rotating)
    if 'contact_shadow' in layers:
        shadow_array = np.array(layers['contact_shadow'])
        rgb = blend_multiply(rgb, shadow_array[:, :, :3], opacity=0.8)

    # Apply glow core (additive blend, non-rotating)
    if 'glow_core' in layers:
        glow_array = np.array(layers['glow_core'])
        rgb = blend_additive(rgb, glow_array[:, :, :3], opacity=0.8)

    # Apply glow crystal (additive blend, non-rotating)
    if 'glow_crystal' in layers:
        crystal_array = np.array(layers['glow_crystal'])
        rgb = blend_additive(rgb, crystal_array[:, :, :3], opacity=0.6)

    # Apply highlight (screen blend, non-rotating)
    if 'highlight' in layers:
        highlight_array = np.array(layers['highlight'])
        rgb = blend_screen(rgb, highlight_array[:, :, :3], opacity=0.5)

    # Apply light wrap (screen blend, non-rotating)
    if 'light_wrap' in layers:
        wrap_array = np.array(layers['light_wrap'])
        rgb = blend_screen(rgb, wrap_array[:, :, :3], opacity=0.4)

    # Apply bloom (screen blend, non-rotating)
    if 'bloom' in layers:
        bloom_array = np.array(layers['bloom'])
        rgb = blend_screen(rgb, bloom_array[:, :, :3], opacity=0.3)

    # Apply indicator (normal blend on top, rotating)
    if 'indicator' in rotated:
        indicator_array = np.array(rotated['indicator'])
        # Use indicator's alpha channel for blending
        ind_alpha = indicator_array[:, :, 3:4] / 255.0
        rgb = (rgb * (1 - ind_alpha) + indicator_array[:, :, :3] * ind_alpha).astype(np.uint8)

    # Combine RGB with alpha
    result_array = np.dstack((rgb, alpha))
    return Image.fromarray(result_array, 'RGBA')


def generate_filmstrip(knob_name: str, num_frames: int = 64, frame_size: int = 512):
    """
    Generate filmstrip for a knob.

    Args:
        knob_name: Knob asset directory name (e.g., 'knob_geode')
        num_frames: Number of rotation positions
        frame_size: Size of each frame in pixels (square)
    """
    print(f"Generating {num_frames}-frame filmstrip for {knob_name}...")

    knob_dir = ASSETS_DIR / knob_name
    if not knob_dir.exists():
        print(f"Error: Knob directory not found: {knob_dir}")
        sys.exit(1)

    # Create output directory
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    # Create circular mask for layers that need it
    print("Creating circular alpha mask...")
    mask = create_circular_alpha_mask(frame_size, feather_pixels=4)

    # Load all PBR layers
    print("Loading PBR layers...")
    layer_files = {
        'albedo': 'albedo.png',
        'ao': 'ao.png',
        'roughness': 'roughness.png',
        'normal': 'normal.png',
        'glow_core': 'glow_core.png',
        'glow_crystal': 'glow_crystal.png',
        'bloom': 'bloom.png',
        'light_wrap': 'light_wrap.png',
        'highlight': 'highlight.png',
        'indicator': 'indicator.png',
        'contact_shadow': 'contact_shadow.png',
    }

    layers = {}
    for name, filename in layer_files.items():
        layer_path = knob_dir / filename
        if layer_path.exists():
            # Apply mask to base layers (albedo, AO, roughness, normal)
            apply_mask = name in ['albedo', 'ao', 'roughness', 'normal']
            layers[name] = load_layer(layer_path, apply_mask=apply_mask, mask=mask)
            print(f"  ✓ Loaded {name}")
        else:
            print(f"  ⚠ Skipped {name} (not found)")

    # Generate frames
    print(f"\nCompositing {num_frames} frames...")
    frames = []
    angle_step = 360.0 / num_frames

    for i in range(num_frames):
        angle = i * angle_step
        frame = composite_frame(layers, angle, frame_size)
        frames.append(frame)

        if (i + 1) % 8 == 0:
            print(f"  Frame {i + 1}/{num_frames} ({angle:.1f}°)")

    # Create filmstrip (stack vertically)
    print("\nCreating filmstrip...")
    filmstrip_height = frame_size * num_frames
    filmstrip = Image.new("RGBA", (frame_size, filmstrip_height), (0, 0, 0, 0))

    for i, frame in enumerate(frames):
        filmstrip.paste(frame, (0, i * frame_size))

    # Save filmstrip
    output_path = OUTPUT_DIR / f"{knob_name}_filmstrip.png"
    print(f"Saving filmstrip to {output_path}...")
    filmstrip.save(output_path, optimize=True)

    file_size_mb = output_path.stat().st_size / (1024 * 1024)
    print(f"✓ Done! Filmstrip size: {file_size_mb:.2f} MB")
    print(f"  Frames: {num_frames}")
    print(f"  Frame size: {frame_size}x{frame_size}px")
    print(f"  Total dimensions: {frame_size}x{filmstrip_height}px")


def main():
    knob_name = sys.argv[1] if len(sys.argv) > 1 else "knob_geode"

    print("=" * 60)
    print("Monument Reverb - Filmstrip Generator")
    print("=" * 60)
    print()

    generate_filmstrip(knob_name, num_frames=64, frame_size=512)

    print()
    print("Next steps:")
    print("1. Use FilmstripKnob component in JUCE")
    print("2. Zero CPU cost at runtime - just image blit!")
    print()


if __name__ == "__main__":
    main()
