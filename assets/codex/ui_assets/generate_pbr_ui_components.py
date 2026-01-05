"""
Utility script to automate generation of PBR‑ready UI components using OpenAI's
image APIs and optional PBR map conversion.

This script demonstrates how to:

  • Call OpenAI's image API to generate a top‑down view of a UI component with
    precise framing and lighting suitable for PBR decomposition.  The prompt
    encourages neutral albedo, soft shadows and a pure white background.  If
    style reference images are provided, the API will apply style transfer to
    match the look and feel of your existing UI.  See the OpenAI prompting
    guide for guidelines on describing what should remain constant versus what
    should change in style transfer【763126881281460†L371-L377】.

  • Iterate over a list of components and material/color variations to produce
    consistent families of assets.

  • Optionally request additional passes (roughness, metalness, ambient
    occlusion, normal map) by adjusting the prompt.  While GPT‑image can
    approximate these maps, most practitioners generate accurate PBR maps by
    feeding the albedo into a dedicated tool.  For example, 3D AI Studio's
    online PBR map generator accepts an image and instantly returns normal,
    roughness, height, metallic and ambient occlusion maps【116002667750075†L90-L92】.
    You can upload the generated albedo images there to retrieve the full
    material set.

To use this script:
  1. Install the OpenAI Python library:
       pip install openai
  2. Set the `OPENAI_API_KEY` environment variable with your API key.
  3. Place any style reference PNGs in a folder and list their file names in
     the `style_paths` list.
  4. Define the components you want to generate in the `components` list.
  5. Run the script.  Images will be saved in a `generated_ui_components`
     directory.

Note: This script only illustrates the API calls and file management.  You
should tune prompts (phrasing, size, seed) to achieve the desired artistic
style and consider adding error handling, logging and rate‑limit backoff for
production use.
"""

import os
import base64
import time
from typing import List

import openai


def load_style_images(style_paths: List[str]) -> List[str]:
    """Read style reference images and return base64‑encoded strings.

    GPT‑image supports multi‑image inputs for style transfer; each image
    influences the output according to its index in the prompt【763126881281460†L115-L124】.
    Reading images once up front avoids repeated disk I/O on subsequent
    requests.

    Args:
        style_paths: list of file paths to PNG or JPEG images.

    Returns:
        List of base64 strings suitable for the API.
    """
    images = []
    for path in style_paths:
        with open(path, "rb") as f:
            encoded = base64.b64encode(f.read()).decode("utf-8")
            images.append(encoded)
    return images


def call_image_api(
    client: openai.OpenAI,
    prompt: str,
    style_images: List[str],
    n: int = 1,
    size: str = "1024x1024",
    quality: str = "standard",
) -> List[bytes]:
    """Invoke the OpenAI image API for generation or editing.

    This function uses `client.images.edit` when style reference images are
    provided and `client.images.generate` otherwise.  The API returns
    base64‑encoded images which are decoded into bytes for saving to disk.

    Args:
        client: authenticated OpenAI client.
        prompt: textual description of the desired image.
        style_images: list of base64‑encoded style images; empty for no style.
        n: number of images to generate.
        size: resolution in the format "WxH".  Common sizes include
            "512x512", "1024x1024" and "2048x2048".
        quality: "standard" or "hd"; HD quality costs more tokens but yields
            sharper details.

    Returns:
        List of raw PNG image bytes.
    """
    # Decide whether to call generate or edit based on style image presence
    if style_images:
        response = client.images.edit(
            model="gpt-image-1.5",  # adjust if using a different model
            prompt=prompt,
            images=style_images,
            n=n,
            size=size,
            quality=quality,
        )
    else:
        response = client.images.generate(
            model="gpt-image-1.5",
            prompt=prompt,
            n=n,
            size=size,
            quality=quality,
        )

    # Each entry in `response.data` contains a base64 string.  Decode to bytes.
    results = []
    for i, data in enumerate(response.data):
        image_bytes = base64.b64decode(data.url.split(",")[-1])  # decode base64
        results.append(image_bytes)
    return results


def generate_components(
    components: List[str],
    style_paths: List[str],
    output_dir: str = "generated_ui_components",
    maps_to_generate: List[str] = None,
    delay: float = 2.0,
) -> None:
    """Generate images for a list of UI components with optional PBR maps.

    Args:
        components: list of component names, e.g. ["rotary knob", "toggle switch"].
        style_paths: file paths to style reference images.
        output_dir: directory where images should be saved.
        maps_to_generate: list of additional passes ("albedo", "roughness",
            "metallic", "normal", "ao").  If None, only the base colour
            render will be produced.  Each map will be generated by issuing a
            separate API call with a tailored prompt.  Note that the API
            output is approximated; for production, you may prefer to use a
            dedicated map generator【116002667750075†L90-L92】.
        delay: pause between API calls to respect rate limits.
    """
    # Ensure output directory exists
    os.makedirs(output_dir, exist_ok=True)

    # Initialise OpenAI client
    client = openai.OpenAI()

    # Load style images once
    style_images = load_style_images(style_paths) if style_paths else []

    # Define base prompts for each map type
    map_prompts = {
        "albedo": "Generate the albedo (base color) map of the top‑down view of a {component} with neutral lighting and no shading. The surface colours should match the provided style reference.",
        "roughness": "Generate a roughness map for the {component}: a grayscale image where darker tones represent rougher areas (e.g., knurled grip) and brighter tones represent smoother areas (e.g., polished metal). Maintain the same orientation and framing as the albedo map.",
        "metallic": "Generate a metallic map for the {component}: a black‑and‑white image where white indicates metal surfaces and black indicates non‑metal surfaces (e.g., plastic knobs). Align perfectly with the albedo map.",
        "normal": "Generate a tangent‑space normal map of the {component}: use the standard RGB colour convention (X in red, Y in green, Z in blue) to encode the surface normals, including fine details like knurling. Top‑down view, matching the albedo map.",
        "ao": "Generate an ambient occlusion map for the {component}: a grayscale image where crevices and edges are darker and open surfaces are lighter. Ensure perfect alignment with the albedo map."
    }

    # Default to only base albedo if maps_to_generate is not provided
    if maps_to_generate is None:
        maps_to_generate = ["albedo"]

    for component in components:
        # Generate base colour image first (albedo) to serve as visual reference
        print(f"Generating {component} base image...")
        base_prompt = (
            f"Top‑down orthographic view of a {component} in the specified style. "
            "Use a neutral white background and soft, even studio lighting. The "
            "component should be perfectly centred with 90 % frame fill and display "
            "the typical features of the part (e.g., knurled grip for a knob, lever for a toggle). "
            "Avoid perspective distortion and ensure the silhouette is circular or rectangular as appropriate."
        )
        images = call_image_api(client, base_prompt, style_images)
        # Save the base image
        base_path = os.path.join(output_dir, f"{component.replace(' ', '_')}_base.png")
        with open(base_path, "wb") as f:
            f.write(images[0])
        print(f"Saved base image to {base_path}")

        # Generate additional maps if requested
        for map_type in maps_to_generate:
            # Skip base because we just generated it
            if map_type == "albedo":
                continue
            map_prompt = map_prompts[map_type].format(component=component)
            print(f"Generating {map_type} map for {component}...")
            map_images = call_image_api(client, map_prompt, style_images)
            map_path = os.path.join(
                output_dir, f"{component.replace(' ', '_')}_{map_type}.png"
            )
            with open(map_path, "wb") as f:
                f.write(map_images[0])
            print(f"Saved {map_type} map to {map_path}")
            time.sleep(delay)


if __name__ == "__main__":
    # Example usage: generate several components in a custom style
    components = [
        "rotary knob",
        "toggle switch",
        "meter gauge",
        "control panel",
        "LED indicator",
        "button",
        "fader slider",
    ]
    # Update these paths to point to your style reference images
    style_paths = ["style_ref_1.png", "style_ref_2.png"]
    generate_components(
        components=components,
        style_paths=style_paths,
        maps_to_generate=["albedo", "roughness", "metallic", "normal", "ao"],
    )