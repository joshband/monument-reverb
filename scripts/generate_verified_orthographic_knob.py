#!/usr/bin/env python3
"""
Generate orthographic knob with vision-based verification.
Regenerates until the image passes orthographic quality checks.
"""

import os
import base64
import urllib.request
from openai import OpenAI

def generate_knob(client, prompt, size="1024x1024", quality="hd"):
    """Generate knob image."""
    response = client.images.generate(
        model="dall-e-3",
        prompt=prompt,
        n=1,
        size=size,
        quality=quality,
    )
    image_url = response.data[0].url
    with urllib.request.urlopen(image_url) as url_response:
        return url_response.read()

def verify_orthographic(client, image_data):
    """Use vision to check if image is truly orthographic."""

    # Encode image to base64
    image_b64 = base64.b64encode(image_data).decode('utf-8')

    verification_prompt = """
Analyze this industrial knob image and determine if it's a truly orthographic
(perfectly flat top-down) view or if it has perspective/3D depth.

Check for these issues that indicate NON-orthographic view:
1. Can you see the side wall as a 3D cylinder (not just a flat ring)?
2. Are there shadows or highlights that suggest viewing angle?
3. Does the knurled edge appear raised/3D rather than flat pattern?
4. Is there any perspective distortion (elliptical instead of circular)?
5. Can you see depth/height information in the image?

Respond with JSON:
{
  "is_orthographic": true/false,
  "confidence": 0.0-1.0,
  "issues": ["list", "of", "problems"],
  "reason": "brief explanation"
}
"""

    response = client.chat.completions.create(
        model="gpt-4o",
        messages=[
            {
                "role": "user",
                "content": [
                    {"type": "text", "text": verification_prompt},
                    {
                        "type": "image_url",
                        "image_url": {
                            "url": f"data:image/png;base64,{image_b64}"
                        }
                    }
                ]
            }
        ],
        max_tokens=500
    )

    import json
    result_text = response.choices[0].message.content

    # Extract JSON from response (handle markdown code blocks)
    if "```json" in result_text:
        result_text = result_text.split("```json")[1].split("```")[0].strip()
    elif "```" in result_text:
        result_text = result_text.split("```")[1].split("```")[0].strip()

    return json.loads(result_text)

def main():
    """Generate and verify orthographic knob."""
    client = OpenAI(api_key=os.environ.get("OPENAI_API_KEY"))

    base_prompt = """
A perfectly flat orthographic technical illustration of an industrial rotary knob.
This is a 2D technical drawing, NOT a 3D rendering or photograph.

CRITICAL REQUIREMENTS:
- ZERO perspective - perfectly parallel projection from infinite distance
- The knob appears as a flat circle, like a coin lying flat on a scanner
- No visible depth, no 3D cylinder effect
- No shadows, no highlights, no shading that suggests viewing angle
- The knurled edge is a FLAT decorative ring pattern, not a raised 3D surface

The knob specifications:
- 30mm diameter circle, brushed aluminum, dark charcoal anodized finish
- Flat top plate with subtle concentric machining lines (2D radial pattern)
- Knurled edge as thin decorative ring around perimeter (flat pattern, not 3D)
- White indicator line at 12 o'clock (thin engraved line)
- Pure white background (#FFFFFF), 90% frame fill, centered

Lighting: Completely flat and neutral. No gradients. No depth cues.

Style: CAD technical drawing, schematic illustration, 2D pattern, flat design.
Think "texture map for 3D model" not "photograph of real object".
    """.strip()

    max_attempts = 3
    output_dir = "assets/ui/hero_knob_pbr"
    os.makedirs(output_dir, exist_ok=True)

    for attempt in range(1, max_attempts + 1):
        print(f"\n{'='*60}")
        print(f"Attempt {attempt}/{max_attempts}")
        print(f"{'='*60}")

        # Add emphasis on each attempt
        if attempt == 1:
            prompt = base_prompt
        elif attempt == 2:
            prompt = base_prompt + "\n\nEMPHASIS: This must be completely flat like a 2D icon or diagram. No 3D depth whatsoever."
        else:
            prompt = base_prompt + "\n\nFINAL ATTEMPT: Create as a pure 2D flat design - imagine you're drawing this in Illustrator or AutoCAD, not rendering in 3D. Absolutely no perspective or depth."

        print("Generating image...")
        image_data = generate_knob(client, prompt)

        # Save attempt
        attempt_path = os.path.join(output_dir, f"albedo_attempt_{attempt}.png")
        with open(attempt_path, 'wb') as f:
            f.write(image_data)
        print(f"Saved attempt to: {attempt_path}")

        print("Verifying orthographic quality with vision...")
        verification = verify_orthographic(client, image_data)

        print(f"\nVerification Result:")
        print(f"  Is Orthographic: {verification['is_orthographic']}")
        print(f"  Confidence: {verification['confidence']:.2f}")
        print(f"  Reason: {verification['reason']}")

        if verification['issues']:
            print(f"  Issues found:")
            for issue in verification['issues']:
                print(f"    - {issue}")

        if verification['is_orthographic'] and verification['confidence'] >= 0.7:
            print(f"\n✅ SUCCESS! Orthographic knob generated on attempt {attempt}")

            # Save as final
            final_path = os.path.join(output_dir, "albedo_orthographic_verified.png")
            with open(final_path, 'wb') as f:
                f.write(image_data)
            print(f"Saved final to: {final_path}")

            print("\nNext steps:")
            print("1. mv assets/ui/hero_knob_pbr/albedo_orthographic_verified.png assets/ui/hero_knob_pbr/albedo.png")
            print("2. python3 scripts/pack_hero_knob_pbr.py")
            print("3. Rebuild plugin")

            return True

        print(f"\n❌ Attempt {attempt} failed verification. ", end="")
        if attempt < max_attempts:
            print("Regenerating with stricter prompt...")
        else:
            print("Max attempts reached.")

    print("\n⚠️  Failed to generate acceptable orthographic view after all attempts.")
    print("You may need to:")
    print("1. Try a different image model")
    print("2. Use a different generation service")
    print("3. Create the texture manually in Photoshop/GIMP")
    print(f"\nAttempt images saved in: {output_dir}/")

    return False

if __name__ == "__main__":
    success = main()
    exit(0 if success else 1)
