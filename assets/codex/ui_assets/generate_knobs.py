"""
Script to programmatically generate industrial rotary knob images using OpenAI's image
generation API.  The script loops over a set of material presets, colourways and
indicator settings, constructs a descriptive prompt for each combination, sends
the request to OpenAI's image API and saves the resulting PNG.  This allows
automated batch generation of assets similar to those created interactively.

Prerequisites:
  * Install the ``openai`` Python package (pip install openai).
  * Set your OpenAI API key in the ``OPENAI_API_KEY`` environment variable or
    pass it explicitly when instantiating the ``OpenAI`` client.
  * Adjust the ``model``, ``size`` and ``quality`` parameters as needed.

The code follows the official OpenAI API example for image generation.  The
example illustrates calling ``client.images.generate`` with the model name,
prompt and optional parameters, and then decoding the returned base64 string
into a PNG file【774656143923385†L379-L411】.
"""

import os
import time
import base64
from pathlib import Path
from openai import OpenAI


def build_prompt(material: str, colour: str, indicator: bool) -> str:
    """Construct the image prompt for a given material, colour and indicator.

    Args:
        material: The material preset (e.g. "brushed aluminum").
        colour: A colour description (e.g. "neutral grey", "charcoal").
        indicator: Whether to include an indicator line on the knob.

    Returns:
        A descriptive prompt string suitable for the image generation API.
    """
    indicator_phrase = (
        "A thin indicator line is present on the top plate."
        if indicator
        else "There is no indicator line on the top surface."
    )
    return (
        f"A flat lay top‑down orthographic product photograph of a single industrial rotary "
        f"knob made of {material} in a {colour} colour. The knob is circular with a flat top "
        f"plate and a knurled cylindrical side wall. It fills about 90% of a square white "
        f"background and is centred. The top surface has fine concentric machining lines and "
        f"a subtle chamfer on the edge. {indicator_phrase} The view is directly overhead with "
        f"no perspective distortion or shadows beyond the knob. Soft, even lighting creates "
        f"smooth highlights without dramatic shadows or rim lighting. There is no text or "
        f"other decoration."
    )


def generate_image(client: OpenAI, prompt: str, model: str = "gpt-image-1.5", size: str = "1024x1024") -> bytes:
    """Generate a single image from a prompt using the OpenAI API.

    This function wraps the call to ``client.images.generate``, as shown in
    the official API example【774656143923385†L379-L411】.  It returns the raw
    bytes of the generated PNG.

    Args:
        client: An instance of ``openai.OpenAI``.
        prompt: The text prompt describing the desired image.
        model: The image model to use (default is ``gpt-image-1.5``).
        size: The size of the generated image (e.g. ``"1024x1024"``).

    Returns:
        A ``bytes`` object containing the PNG image data.
    """
    result = client.images.generate(
        model=model,
        prompt=prompt,
        n=1,
        size=size,
    )
    # The GPT image models return base64-encoded image data in the ``b64_json`` field.
    image_base64 = result.data[0].b64_json
    return base64.b64decode(image_base64)


def save_image(data: bytes, path: Path) -> None:
    """Write binary image data to disk at the given path, creating parent dirs."""
    path.parent.mkdir(parents=True, exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)


def main() -> None:
    """Generate knob images for each material, colour and indicator combination."""
    # Define materials and colourways to iterate over
    materials = [
        "brushed aluminum",
        "anodized aluminum (matte)",
        "anodized aluminum (gloss)",
        "powder‑coated steel",
        "injection‑molded polymer",
        "bakelite‑style phenolic",
        "rubberized soft‑touch",
        "raw machined steel",
    ]
    colours = [
        "neutral grey",
        "charcoal",
        "black",
        "off‑white",
        "olive",
        "navy",
        "oxide red",
    ]
    indicator_options = [True, False]

    # Create OpenAI client. The API key can be set via the environment variable.
    api_key = os.environ.get("OPENAI_API_KEY")
    if api_key:
        client = OpenAI(api_key=api_key)
    else:
        # Instantiate without explicit key to use default mechanism (e.g. openai.cfg file)
        client = OpenAI()

    output_dir = Path("generated_knobs")

    # Iterate over combinations
    for material in materials:
        for colour in colours:
            for indicator in indicator_options:
                prompt = build_prompt(material, colour, indicator)
                print(f"Generating: {material}, {colour}, indicator={indicator}")
                try:
                    image_data = generate_image(client, prompt)
                except Exception as e:
                    print(f"Failed to generate image for {material}/{colour}/indicator={indicator}: {e}")
                    continue
                # Build filename
                material_slug = material.replace(" ", "_").replace("(", "").replace(")", "").replace("‑", "-")
                colour_slug = colour.replace(" ", "_").replace("‑", "-")
                indicator_slug = "with_indicator" if indicator else "no_indicator"
                filename = f"{material_slug}_{colour_slug}_{indicator_slug}.png"
                save_image(image_data, output_dir / filename)
                # Polite pause to avoid hitting rate limits
                time.sleep(2)
    print(f"Finished generating knob images. Files saved in {output_dir.resolve()}.")


if __name__ == "__main__":
    main()