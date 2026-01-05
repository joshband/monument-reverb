#!/usr/bin/env python3
"""
Generate a Monument Reverb-style industrial rotary knob with full PBR maps
using OpenAI's image API.

Generates:
- albedo.png (base color texture)
- roughness.png (surface roughness)
- metallic.png (metallic vs non-metallic)
- normal.png (surface detail normal map)
- ao.png (ambient occlusion for depth)
"""

import os
import base64
import time
from openai import OpenAI

def generate_image(client, prompt, size="1024x1024", quality="hd"):
    """Generate image using OpenAI API."""
    print(f"Generating: {prompt[:80]}...")

    response = client.images.generate(
        model="dall-e-3",
        prompt=prompt,
        n=1,
        size=size,
        quality=quality,
    )

    # Download image from URL
    import urllib.request
    image_url = response.data[0].url
    with urllib.request.urlopen(image_url) as url_response:
        return url_response.read()

def save_image(data, path):
    """Save image bytes to file."""
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'wb') as f:
        f.write(data)
    print(f"  Saved: {path}")

def main():
    """Generate Monument knob with PBR maps."""

    # Initialize OpenAI client
    client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY"))

    output_dir = "assets/ui/hero_knob_pbr"
    os.makedirs(output_dir, exist_ok=True)

    # Define prompts for each map
    prompts = {
        "albedo": """
            A top-down orthographic product photograph of a single industrial rotary knob
            for audio equipment. The knob is made of brushed aluminum with a dark charcoal
            anodized finish. It has a circular flat top plate (30mm diameter) with fine
            concentric machining lines and a knurled cylindrical side wall. A thin white
            indicator line is engraved on the top surface pointing toward 12 o'clock.
            The knob fills 90% of the frame on a pure white background (#FFFFFF), centered
            perfectly. View is directly overhead with no perspective distortion. Soft, even
            studio lighting with no dramatic shadows or highlights. The surface shows subtle
            brushed metal texture with neutral albedo colors (no lighting information baked in).
            Professional product photography style, high detail, no text or decoration.
        """,

        "roughness": """
            A grayscale roughness map for an industrial aluminum rotary knob, top-down view.
            Darker values (0.2-0.4) represent the smooth brushed aluminum top plate with
            fine concentric machining lines. Brighter values (0.6-0.8) represent the
            rougher knurled grip on the cylindrical side wall. The indicator line area
            is slightly rougher (0.5) due to engraving. Pure grayscale from black (smooth)
            to white (rough). No lighting, no shadows, just material roughness information.
            Matches the exact framing and orientation of the albedo map.
        """,

        "metallic": """
            A black and white metallic map for an industrial aluminum rotary knob, top-down view.
            The entire knob is metal, so the image is almost entirely white (metallic=1.0).
            Only the indicator line may show slight variation. Pure binary metallic information:
            white = metal surfaces, black = non-metal. Matches the exact framing of the albedo map.
            No gradients, no lighting, just material classification.
        """,

        "normal": """
            A tangent-space normal map for an industrial aluminum rotary knob, top-down view.
            Use standard RGB encoding: X-axis in red channel, Y-axis in green channel, Z-axis
            in blue channel. The base color is RGB(128, 128, 255) representing flat surface.
            Concentric machining lines on the top plate create subtle radial variations.
            The knurled side wall shows pronounced bumps (brighter blue for raised areas).
            The engraved indicator line is a subtle depression (slightly darker blue).
            Matches exact framing of the albedo map. Technical normal map, not a photograph.
        """,

        "ao": """
            A grayscale ambient occlusion map for an industrial aluminum rotary knob, top-down view.
            Bright areas (0.9-1.0) for open flat surfaces like the top plate center.
            Darker areas (0.3-0.5) for crevices: the edge where top plate meets knurled wall,
            grooves in the knurled texture, and the engraved indicator line. Medium values (0.6-0.8)
            for the general knurled surface. No direct lighting, only cavity occlusion information.
            Matches exact framing of the albedo map. Pure grayscale ambient occlusion data.
        """
    }

    # Generate each map with delay between requests
    for map_name, prompt in prompts.items():
        try:
            print(f"\nGenerating {map_name} map...")
            image_data = generate_image(client, prompt.strip())
            save_image(image_data, os.path.join(output_dir, f"{map_name}.png"))

            # Polite delay between API calls (except after last one)
            if map_name != "ao":
                print(f"  Waiting 10 seconds before next request...")
                time.sleep(10)

        except Exception as e:
            print(f"ERROR generating {map_name}: {e}")
            return False

    print(f"\n✅ All PBR maps generated successfully in {output_dir}/")
    print(f"\nNext steps:")
    print(f"1. Review the generated maps")
    print(f"2. Run: python3 scripts/pack_hero_knob_pbr.py")
    print(f"3. Rebuild the plugin")

    return True

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
