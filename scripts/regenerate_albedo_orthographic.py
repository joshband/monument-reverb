#!/usr/bin/env python3
"""
Regenerate albedo with strict orthographic (perfectly flat top-down) view.
"""

import os
import urllib.request
from openai import OpenAI

def main():
    client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY"))

    # Ultra-strict orthographic prompt - no 3D depth, pure flat top-down
    prompt = """
A perfectly flat, orthographic top-down technical illustration of an industrial
rotary knob for audio equipment, as if photographed with a telecentric lens
eliminating all perspective distortion.

The knob is a precise circle (30mm diameter) made of brushed aluminum with dark
charcoal anodized finish. The view is EXACTLY perpendicular - looking straight
down from directly above with zero viewing angle.

The knob has:
- A completely flat circular top plate with fine concentric machining lines (radial pattern)
- A knurled cylindrical side wall visible only as a thin outer ring (no 3D depth)
- A thin white indicator line pointing toward 12 o'clock (engraved into the surface)

The knob fills 90% of the frame on a pure white background (#FFFFFF), perfectly
centered. The lighting is completely flat and neutral - no highlights, no shadows,
no shading gradients that would imply depth or viewing angle. This should look
like a technical CAD drawing or a scan, not a photograph with depth.

The surface shows subtle brushed aluminum texture in the charcoal color, but
with NO lighting information baked in. Think "texture map" not "rendered image".

The knurled edge appears as a thin decorative ring pattern around the perimeter,
not as a 3D raised edge.

Style: Industrial design technical drawing, perfectly flat orthographic projection,
zero perspective, no shadows, neutral even lighting, high detail.
    """.strip()

    print("Generating perfectly orthographic albedo...")
    print("(This may take 20-30 seconds)")

    response = client.images.generate(
        model="dall-e-3",
        prompt=prompt,
        n=1,
        size="1024x1024",
        quality="hd",
    )

    # Download image
    image_url = response.data[0].url
    with urllib.request.urlopen(image_url) as url_response:
        image_data = url_response.read()

    # Save
    output_path = "assets/ui/hero_knob_pbr/albedo_orthographic.png"
    os.makedirs(os.path.dirname(output_path), exist_ok=True)
    with open(output_path, 'wb') as f:
        f.write(image_data)

    print(f"✓ Saved: {output_path}")
    print("\nNext: Run python3 scripts/pack_hero_knob_pbr.py to process with alpha")

if __name__ == "__main__":
    main()
