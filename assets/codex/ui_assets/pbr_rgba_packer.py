"""
Utilities for packing and unpacking PBR texture maps into a single RGBA image
and creating a JSON description of the composite.  Packing ambient occlusion,
roughness, and metalness into the red, green and blue channels of one
texture (an “ORM map”) improves performance by reducing the number of
textures that need to be loaded in real‑time rendering【536554926942047†L51-L84】.  Each
channel stores grayscale data at the same resolution to ensure consistent
alignment【536554926942047†L93-L98】.  The optional alpha channel can hold a height
map, specular mask or be left empty.

The module requires Pillow (PIL) for image manipulation:

    pip install pillow

Usage examples:

    # Pack separate roughness, metalness, ambient occlusion and height maps
    pack_to_rgba(
        roughness_path="rough.png",
        metalness_path="metal.png",
        ao_path="ao.png",
        height_path="height.png",
        output_path="packed_ormh.png"
    )

    # Unpack an RGBA image into separate grayscale maps
    unpack_from_rgba("packed_ormh.png", "out_dir")

    # Create a JSON manifest describing the composite
    schema = create_composite_manifest(
        albedo_path="albedo.png",
        normal_path="normal.png",
        packed_path="packed_ormh.png",
        emissive_path="emissive.png",
        opacity_path=None,
        channel_map={"R": "roughness", "G": "metalness", "B": "ambientOcclusion", "A": "height"}
    )
    write_manifest(schema, "component_manifest.json")

"""

import os
from typing import Dict, Optional

from PIL import Image


def load_grayscale(path: str, resolution: Optional[tuple] = None) -> Image.Image:
    """
    Load an image as a single‑channel (L mode) grayscale image.  Optionally
    resample to match the desired resolution.

    Args:
        path: Path to the input image.
        resolution: Desired resolution (width, height).  If None, the image
            retains its original size.

    Returns:
        A Pillow Image in L mode.
    """
    img = Image.open(path).convert("L")
    if resolution and img.size != resolution:
        img = img.resize(resolution, Image.LANCZOS)
    return img


def pack_to_rgba(
    roughness_path: str,
    metalness_path: str,
    ao_path: str,
    height_path: Optional[str],
    output_path: str,
) -> None:
    """
    Pack four grayscale maps (roughness, metalness, ambient occlusion and height)
    into a single RGBA image.  If `height_path` is None, the alpha channel will
    contain fully opaque (255) values.

    The red, green and blue channels store roughness, metalness and ambient
    occlusion respectively; the alpha channel stores height or remains fully
    opaque.  These assignments correspond to the typical ORM workflow where
    each channel contains a separate grayscale map【536554926942047†L51-L84】.

    Args:
        roughness_path: Path to the roughness map (grayscale).
        metalness_path: Path to the metalness map (grayscale).
        ao_path: Path to the ambient occlusion map (grayscale).
        height_path: Path to the height map (grayscale) or None.
        output_path: Destination file path for the packed RGBA texture.
    """
    # Load each map as grayscale
    r = load_grayscale(roughness_path)
    g = load_grayscale(metalness_path, resolution=r.size)
    b = load_grayscale(ao_path, resolution=r.size)
    if height_path:
        a = load_grayscale(height_path, resolution=r.size)
    else:
        # Create a fully opaque channel if no height map is provided
        a = Image.new("L", r.size, color=255)

    # Merge channels into an RGBA image
    rgba = Image.merge("RGBA", (r, g, b, a))
    rgba.save(output_path)


def unpack_from_rgba(input_path: str, output_dir: str) -> Dict[str, str]:
    """
    Split an RGBA image into four separate grayscale images and save them to
    disk.  The channels are saved as red.png, green.png, blue.png and alpha.png
    in the specified output directory.

    Args:
        input_path: Path to the packed RGBA image.
        output_dir: Directory where the unpacked images should be written.

    Returns:
        A dictionary mapping channel names to file paths.
    """
    img = Image.open(input_path).convert("RGBA")
    channels = img.split()
    names = ["red", "green", "blue", "alpha"]
    os.makedirs(output_dir, exist_ok=True)
    paths = {}
    for name, chan in zip(names, channels):
        path = os.path.join(output_dir, f"{name}.png")
        chan.save(path)
        paths[name] = path
    return paths


def create_composite_manifest(
    albedo_path: str,
    normal_path: str,
    packed_path: str,
    emissive_path: Optional[str] = None,
    opacity_path: Optional[str] = None,
    channel_map: Optional[Dict[str, str]] = None,
) -> Dict:
    """
    Build a dictionary describing a PBR texture set following the
    `pbr_composite_schema.json` specification.  It records the file paths for
    albedo, normal and packed maps and the semantic meaning of each channel.

    Args:
        albedo_path: Path to the albedo/base colour map.
        normal_path: Path to the normal map.
        packed_path: Path to the packed RGBA map.
        emissive_path: Optional path to the emissive map; set to None if not used.
        opacity_path: Optional path to the opacity map; set to None if not used.
        channel_map: Mapping of RGBA channels to semantic names (roughness,
            metalness, ambientOcclusion, height or specular).  Defaults to the
            typical mapping {"R": "roughness", "G": "metalness", "B": "ambientOcclusion", "A": "height"}.

    Returns:
        A dictionary matching the schema requirements.
    """
    if channel_map is None:
        channel_map = {
            "R": "roughness",
            "G": "metalness",
            "B": "ambientOcclusion",
            "A": "height",
        }
    return {
        "albedoMap": albedo_path,
        "normalMap": normal_path,
        "packedMap": {
            "file": packed_path,
            "channels": channel_map,
        },
        "emissiveMap": emissive_path,
        "opacityMap": opacity_path,
    }


def write_manifest(manifest: Dict, output_path: str) -> None:
    """
    Write the manifest dictionary to disk as a JSON file.

    Args:
        manifest: The dictionary returned by `create_composite_manifest`.
        output_path: Destination file path for the JSON manifest.
    """
    import json
    with open(output_path, "w", encoding="utf-8") as f:
        json.dump(manifest, f, indent=2)


def main():
    """
    Demonstration of packing separate PBR maps into a composite RGBA image and
    generating a manifest.  Replace the file paths below with your actual
    images.  When running this script directly, this function will execute.
    """
    # Example input paths (replace these with real files)
    albedo = "albedo.png"
    normal = "normal.png"
    rough = "roughness.png"
    metal = "metalness.png"
    ao = "ambient_occlusion.png"
    height = "height.png"
    emissive = "emissive.png"

    # Create packed RGBA map
    output_packed = "packed_ormh.png"
    pack_to_rgba(rough, metal, ao, height, output_packed)
    print(f"Packed maps into {output_packed}")

    # Create manifest dictionary
    manifest = create_composite_manifest(
        albedo_path=albedo,
        normal_path=normal,
        packed_path=output_packed,
        emissive_path=emissive,
        opacity_path=None,
    )
    manifest_path = "component_manifest.json"
    write_manifest(manifest, manifest_path)
    print(f"Written manifest to {manifest_path}")


if __name__ == "__main__":
    main()