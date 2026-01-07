#!/usr/bin/env python3
"""Generate procedural PBR textures for brushed metal knob."""

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import math

SIZE = 512
CENTER = SIZE // 2

def save_image(data, filename, mode='L'):
    """Save numpy array as PNG image."""
    if data.dtype != np.uint8:
        data = np.clip(data * 255, 0, 255).astype(np.uint8)
    img = Image.fromarray(data, mode=mode)
    img.save(f"assets/knob_metal/{filename}")
    print(f"✓ Generated: {filename}")

def circular_gradient(size, center, inner_radius, outer_radius, inner_val=1.0, outer_val=0.0):
    """Create a circular gradient."""
    y, x = np.ogrid[:size, :size]
    dist = np.sqrt((x - center)**2 + (y - center)**2)

    # Normalize distance to [0, 1] range
    normalized = (dist - inner_radius) / (outer_radius - inner_radius)
    normalized = np.clip(normalized, 0, 1)

    # Smooth interpolation
    gradient = inner_val + (outer_val - inner_val) * normalized
    return gradient

def add_noise(data, amount=0.05):
    """Add subtle noise to texture."""
    noise = np.random.randn(*data.shape) * amount
    return np.clip(data + noise, 0, 1)

def generate_albedo():
    """Brushed metal albedo with circular grain."""
    print("\n1. Generating albedo (brushed metal base)...")

    # Base metal color (steel gray)
    base = np.full((SIZE, SIZE), 0.6)

    # Add circular brushed grain
    y, x = np.ogrid[:SIZE, :SIZE]
    angles = np.arctan2(y - CENTER, x - CENTER)

    # Create circular grain pattern
    grain_frequency = 80
    grain = np.sin(angles * grain_frequency)
    grain = (grain + 1) * 0.5  # Normalize to [0, 1]

    # Blend grain with base
    albedo = base + grain * 0.15

    # Add radial gradient (darker at edges)
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    radial = 1.0 - (dist / CENTER) * 0.3
    radial = np.clip(radial, 0, 1)

    albedo = albedo * radial

    # Add subtle noise
    albedo = add_noise(albedo, 0.02)

    save_image(albedo, "albedo.png")

def generate_ao():
    """Ambient occlusion - darker at edges."""
    print("2. Generating ambient occlusion...")

    # Strong edge darkening
    ao = circular_gradient(SIZE, CENTER, CENTER * 0.7, CENTER * 1.0, 1.0, 0.3)

    # Add subtle center cavity
    center_cavity = circular_gradient(SIZE, CENTER, 0, CENTER * 0.15, 0.85, 1.0)
    ao = np.minimum(ao, center_cavity)

    save_image(ao, "ao.png")

def generate_anisotropic():
    """Anisotropic highlights for brushed metal."""
    print("3. Generating anisotropic highlights...")

    y, x = np.ogrid[:SIZE, :SIZE]
    angles = np.arctan2(y - CENTER, x - CENTER)

    # Create sharp anisotropic streaks
    streak_frequency = 120
    streaks = np.cos(angles * streak_frequency)
    streaks = (streaks + 1) * 0.5
    streaks = np.power(streaks, 3)  # Sharpen

    # Mask to ring area only
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    ring_mask = np.logical_and(dist > CENTER * 0.65, dist < CENTER * 0.95)

    aniso = np.zeros((SIZE, SIZE))
    aniso[ring_mask] = streaks[ring_mask]

    # Add radial fade
    radial_fade = 1.0 - np.abs(dist - CENTER * 0.8) / (CENTER * 0.3)
    radial_fade = np.clip(radial_fade, 0, 1)

    aniso = aniso * radial_fade

    save_image(aniso, "anisotropic.png")

def generate_specular():
    """Sharp specular highlights."""
    print("4. Generating specular highlights...")

    # Top-right highlight
    y, x = np.ogrid[:SIZE, :SIZE]

    # Create directional highlight
    light_dir = np.array([CENTER * 1.4, CENTER * 0.6])
    dx = x - light_dir[0]
    dy = y - light_dir[1]
    dist = np.sqrt(dx**2 + dy**2)

    # Sharp falloff
    specular = 1.0 - (dist / (CENTER * 0.6))
    specular = np.clip(specular, 0, 1)
    specular = np.power(specular, 4)  # Very sharp

    # Mask to ring area
    center_dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    ring_mask = np.logical_and(center_dist > CENTER * 0.65, center_dist < CENTER * 0.95)

    specular = specular * ring_mask * 0.8

    save_image(specular, "specular.png")

def generate_glow():
    """Subtle LED ring glow."""
    print("5. Generating LED ring glow...")

    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    # Ring at specific radius
    ring_radius = CENTER * 0.8
    ring_width = CENTER * 0.08

    ring_dist = np.abs(dist - ring_radius)
    glow = 1.0 - (ring_dist / ring_width)
    glow = np.clip(glow, 0, 1)
    glow = np.power(glow, 2)  # Soften

    # Cyan/blue tint (will be applied via additive blend)
    save_image(glow, "glow.png")

def generate_indicator():
    """Sharp metal indicator pointer."""
    print("6. Generating indicator pointer...")

    img = Image.new('L', (SIZE, SIZE), 0)
    draw = ImageDraw.Draw(img)

    # Indicator points to 3 o'clock (0 degrees)
    # Draw a sharp pointer from center to edge
    pointer_length = CENTER * 0.7
    pointer_width = 8

    # Pointer rectangle
    x1 = CENTER
    y1 = CENTER - pointer_width // 2
    x2 = CENTER + pointer_length
    y2 = CENTER + pointer_width // 2

    draw.rectangle([x1, y1, x2, y2], fill=255)

    # Add rounded end cap
    cap_x = CENTER + pointer_length - 10
    draw.ellipse([cap_x - 12, CENTER - 12, cap_x + 12, CENTER + 12], fill=255)

    # Convert to numpy and normalize
    indicator = np.array(img) / 255.0

    save_image(indicator, "indicator.png")

def generate_contact_shadow():
    """Soft drop shadow beneath knob."""
    print("7. Generating contact shadow...")

    y, x = np.ogrid[:SIZE, :SIZE]

    # Offset shadow down and right slightly
    shadow_center_x = CENTER + 15
    shadow_center_y = CENTER + 15

    dist = np.sqrt((x - shadow_center_x)**2 + (y - shadow_center_y)**2)

    # Large soft shadow
    shadow = 1.0 - (dist / (CENTER * 1.1))
    shadow = np.clip(shadow, 0, 1)
    shadow = np.power(shadow, 1.5)  # Soft falloff

    # Make shadow darker and multiply-friendly
    shadow = 1.0 - shadow * 0.6

    save_image(shadow, "contact_shadow.png")

def generate_roughness():
    """Surface roughness map."""
    print("8. Generating roughness map...")

    # Base medium roughness
    roughness = np.full((SIZE, SIZE), 0.45)

    # Rougher at edges (more worn)
    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    edge_roughness = (dist / CENTER) * 0.3

    roughness = roughness + edge_roughness

    # Add micro-scratches
    scratch_noise = np.random.randn(SIZE, SIZE) * 0.08
    roughness = np.clip(roughness + scratch_noise, 0, 1)

    save_image(roughness, "roughness.png")

def generate_manifest():
    """Create manifest.json for knob_metal."""
    print("\n9. Creating manifest.json...")

    manifest = """{
  "logicalSize": 512,
  "pivot": { "x": 0.5, "y": 0.5 },
  "layers": [
    {
      "name": "contact_shadow",
      "file": "contact_shadow.png",
      "blend": "multiply",
      "opacity": 0.7,
      "rotates": false
    },
    {
      "name": "albedo",
      "file": "albedo.png",
      "blend": "normal",
      "opacity": 1.0,
      "rotates": true
    },
    {
      "name": "ao",
      "file": "ao.png",
      "blend": "multiply",
      "opacity": 0.8,
      "rotates": false
    },
    {
      "name": "roughness",
      "file": "roughness.png",
      "blend": "screen",
      "opacity": 0.25,
      "rotates": false,
      "pulse": true
    },
    {
      "name": "anisotropic",
      "file": "anisotropic.png",
      "blend": "screen",
      "opacity": 0.6,
      "rotates": true
    },
    {
      "name": "specular",
      "file": "specular.png",
      "blend": "screen",
      "opacity": 0.7,
      "rotates": false
    },
    {
      "name": "glow",
      "file": "glow.png",
      "blend": "add",
      "opacity": 0.4,
      "rotates": false,
      "glow": true
    },
    {
      "name": "indicator",
      "file": "indicator.png",
      "blend": "screen",
      "opacity": 0.95,
      "rotates": true,
      "indicator": true
    }
  ]
}
"""

    with open("assets/knob_metal/manifest.json", "w") as f:
        f.write(manifest)

    print("✓ Created: manifest.json")

def main():
    print("=" * 60)
    print("Generating Procedural PBR Textures for Brushed Metal Knob")
    print("=" * 60)

    generate_albedo()
    generate_ao()
    generate_anisotropic()
    generate_specular()
    generate_glow()
    generate_indicator()
    generate_contact_shadow()
    generate_roughness()
    generate_manifest()

    print("\n" + "=" * 60)
    print("✓ All PBR textures generated successfully!")
    print(f"✓ Output directory: assets/knob_metal/")
    print(f"✓ Total layers: 8")
    print("=" * 60)

if __name__ == '__main__':
    main()
