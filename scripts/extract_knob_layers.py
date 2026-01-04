#!/usr/bin/env python3
"""Extract round, alpha-channel knob layers from Materialize PBR output.

Takes PBR maps (albedo, normal, AO, etc.) and creates circular,
alpha-masked PNG layers ready for JUCE rendering.
"""

import argparse
import json
from dataclasses import dataclass
from pathlib import Path
from typing import Optional

import numpy as np
from PIL import Image, ImageDraw, ImageFilter, ImageChops


@dataclass
class LayerConfig:
    """Configuration for extracting a specific layer type."""
    name: str
    base_map: str  # Primary PBR map to use
    blend_maps: list[tuple[str, str, float]]  # (map, blend_mode, opacity)
    edge_feather: int = 2  # Pixels to feather at edge
    center_crop: Optional[float] = None  # Crop to center circle (0.0-1.0 of radius)


# Layer extraction configurations
LAYER_CONFIGS = {
    "base_body": LayerConfig(
        name="base_body",
        base_map="albedo",
        blend_maps=[
            ("ao", "multiply", 0.7),  # Ambient occlusion for depth
            ("normal", "overlay", 0.3),  # Subtle normal map detail
        ],
        edge_feather=4,
    ),
    "indicator": LayerConfig(
        name="indicator",
        base_map="height",  # Use height map to isolate raised indicator
        blend_maps=[
            ("albedo", "normal", 1.0),  # Color from albedo
            ("ao", "multiply", 0.5),
        ],
        edge_feather=2,
    ),
    "detail_ring": LayerConfig(
        name="detail_ring",
        base_map="normal",  # Engravings are strongest in normal map
        blend_maps=[
            ("albedo", "normal", 0.6),
            ("ao", "multiply", 0.4),
        ],
        edge_feather=3,
    ),
    "center_cap": LayerConfig(
        name="center_cap",
        base_map="albedo",
        blend_maps=[
            ("roughness", "overlay", 0.3),
            ("ao", "multiply", 0.6),
        ],
        edge_feather=2,
        center_crop=0.16,  # 80px / 512px = ~16% of radius
    ),
}


def create_circular_mask(size: int, feather: int = 2, center_crop: Optional[float] = None) -> Image.Image:
    """Create a circular alpha mask with optional feathering.

    Args:
        size: Image size (square)
        feather: Pixels to feather at edge (anti-aliasing)
        center_crop: If provided, crop to this fraction of radius (for center caps)

    Returns:
        Grayscale mask image (0=transparent, 255=opaque)
    """
    mask = Image.new("L", (size, size), 0)
    draw = ImageDraw.Draw(mask)

    center = size / 2
    if center_crop:
        radius = center * center_crop
    else:
        radius = center - feather

    # Draw circle
    draw.ellipse(
        [center - radius, center - radius, center + radius, center + radius],
        fill=255
    )

    # Apply Gaussian blur for smooth feathering
    if feather > 0:
        mask = mask.filter(ImageFilter.GaussianBlur(radius=feather / 2))

    return mask


def blend_images(base: Image.Image, overlay: Image.Image, mode: str, opacity: float) -> Image.Image:
    """Blend two images using specified mode and opacity.

    Args:
        base: Base image
        overlay: Overlay image
        mode: Blend mode ("multiply", "overlay", "normal", "screen")
        opacity: Blend opacity (0.0-1.0)

    Returns:
        Blended image
    """
    if mode == "multiply":
        # Multiply: darkens, good for AO
        result = ImageChops.multiply(base, overlay)
    elif mode == "overlay":
        # Overlay: contrast enhancement
        result = Image.blend(base, overlay, 0.5)
        result = ImageChops.screen(result, overlay) if opacity > 0.5 else ImageChops.multiply(result, overlay)
    elif mode == "screen":
        # Screen: lightens
        result = ImageChops.screen(base, overlay)
    else:  # "normal"
        result = overlay

    # Apply opacity
    return Image.blend(base, result, opacity)


def extract_layer(
    pbr_dir: Path,
    config: LayerConfig,
    size: int = 512
) -> Optional[Image.Image]:
    """Extract a single knob layer from PBR maps.

    Args:
        pbr_dir: Directory containing PBR maps (albedo.png, normal.png, etc.)
        config: Layer extraction configuration
        size: Output size (square)

    Returns:
        RGBA image with circular alpha mask, or None if base map not found
    """
    # Load base map
    base_path = pbr_dir / f"{config.base_map}.png"
    if not base_path.exists():
        return None

    base = Image.open(base_path).convert("RGB")
    if base.size != (size, size):
        base = base.resize((size, size), Image.Resampling.LANCZOS)

    # Apply blend maps
    result = base.copy()
    for map_name, blend_mode, opacity in config.blend_maps:
        map_path = pbr_dir / f"{map_name}.png"
        if map_path.exists():
            overlay = Image.open(map_path).convert("RGB")
            if overlay.size != (size, size):
                overlay = overlay.resize((size, size), Image.Resampling.LANCZOS)
            result = blend_images(result, overlay, blend_mode, opacity)

    # Create circular alpha mask
    mask = create_circular_mask(size, config.edge_feather, config.center_crop)

    # Convert to RGBA and apply mask
    result = result.convert("RGBA")
    result.putalpha(mask)

    return result


def process_job(
    input_dir: Path,
    output_dir: Path,
    job_id: str,
    size: int = 512
) -> dict:
    """Process a single Materialize job into knob layers.

    Args:
        input_dir: Materialize output directory for this job
        output_dir: Output directory for extracted layers
        job_id: Job identifier
        size: Output size

    Returns:
        Processing result dictionary
    """
    result = {
        "job_id": job_id,
        "layers_extracted": [],
        "errors": [],
    }

    # Determine layer type from job_id
    layer_type = None
    for layer_name in LAYER_CONFIGS.keys():
        if layer_name in job_id:
            layer_type = layer_name
            break

    if not layer_type:
        result["errors"].append(f"Could not determine layer type from job_id: {job_id}")
        return result

    config = LAYER_CONFIGS[layer_type]

    # Extract layer
    layer_image = extract_layer(input_dir, config, size)
    if layer_image is None:
        result["errors"].append(f"Failed to extract layer (base map '{config.base_map}' not found)")
        return result

    # Save layer
    output_dir.mkdir(parents=True, exist_ok=True)
    output_path = output_dir / f"{job_id}.png"
    layer_image.save(output_path, "PNG")

    result["layers_extracted"].append({
        "layer_type": layer_type,
        "output_path": str(output_path),
        "size": f"{size}x{size}",
    })

    # Save metadata
    metadata = {
        "job_id": job_id,
        "layer_type": layer_type,
        "config": {
            "base_map": config.base_map,
            "blend_maps": [
                {"map": m, "mode": mode, "opacity": op}
                for m, mode, op in config.blend_maps
            ],
            "edge_feather": config.edge_feather,
            "center_crop": config.center_crop,
        },
        "output_file": str(output_path),
    }

    metadata_path = output_dir / f"{job_id}.json"
    with metadata_path.open("w") as f:
        json.dump(metadata, f, indent=2)

    return result


def main() -> int:
    """CLI entrypoint."""
    parser = argparse.ArgumentParser(
        description="Extract round, alpha-channel knob layers from Materialize PBR output"
    )
    parser.add_argument(
        "--in",
        dest="input_dir",
        required=True,
        help="Materialize output directory (contains job subdirectories)"
    )
    parser.add_argument(
        "--out",
        dest="output_dir",
        required=True,
        help="Output directory for extracted layers"
    )
    parser.add_argument(
        "--size",
        type=int,
        default=512,
        help="Output layer size in pixels (default: 512)"
    )

    args = parser.parse_args()

    input_root = Path(args.input_dir)
    output_root = Path(args.output_dir)

    if not input_root.exists():
        print(f"❌ Input directory does not exist: {input_root}")
        return 1

    # Discover Materialize job directories
    job_dirs = sorted([
        d for d in input_root.iterdir()
        if d.is_dir() and not d.name.startswith(".")
    ])

    if not job_dirs:
        print(f"❌ No job directories found in {input_root}")
        return 1

    print(f"🎨 Extracting knob layers from {len(job_dirs)} jobs")
    print(f"📁 Input: {input_root}")
    print(f"📁 Output: {output_root}")
    print(f"📐 Size: {args.size}x{args.size}\n")

    results = []
    for job_dir in job_dirs:
        job_id = job_dir.name
        print(f"Processing {job_id}...", end=" ")

        result = process_job(job_dir, output_root, job_id, args.size)
        results.append(result)

        if result["errors"]:
            print(f"❌ {result['errors'][0]}")
        else:
            print(f"✅ {len(result['layers_extracted'])} layer(s) extracted")

    # Summary
    successful = sum(1 for r in results if not r["errors"])
    failed = len(results) - successful

    print("\n" + "="*60)
    print(f"✨ Extraction complete!")
    print(f"   Successful: {successful}")
    print(f"   Failed: {failed}")

    if failed > 0:
        print(f"\n⚠️  Failed jobs:")
        for r in results:
            if r["errors"]:
                print(f"   - {r['job_id']}: {r['errors'][0]}")

    # Save manifest
    manifest_path = output_root / "extraction_manifest.json"
    with manifest_path.open("w") as f:
        json.dump({
            "extraction_summary": {
                "total_jobs": len(results),
                "successful": successful,
                "failed": failed,
                "output_size": f"{args.size}x{args.size}",
            },
            "results": results,
        }, f, indent=2)

    print(f"\n📄 Manifest saved: {manifest_path}")
    print("="*60)

    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())