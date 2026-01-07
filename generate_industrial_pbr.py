#!/usr/bin/env python3
"""
Generate procedural PBR layers for industrial control knob.
Inspired by professional audio equipment knobs with:
- Fine-grain textured surface (leather-like)
- Circular tick marks around outer ring
- Metallic bezel rim
- Indicator dots
- Deep contact shadows
"""

import numpy as np
from PIL import Image, ImageDraw, ImageFilter
import json
import os

SIZE = 512
CENTER = SIZE // 2
RADIUS = SIZE // 2 - 20  # Leave room for bezel


def create_circular_gradient(size, center, inner_radius, outer_radius, invert=False):
    """Create radial gradient between two radii."""
    y, x = np.ogrid[:size, :size]
    dist = np.sqrt((x - center)**2 + (y - center)**2)

    mask = np.zeros((size, size), dtype=np.float32)
    in_ring = (dist >= inner_radius) & (dist <= outer_radius)

    if np.any(in_ring):
        normalized = (dist[in_ring] - inner_radius) / (outer_radius - inner_radius)
        if invert:
            normalized = 1.0 - normalized
        mask[in_ring] = normalized

    return mask


def create_fine_grain_texture(size, scale=0.5, octaves=4):
    """Create fine leather-like grain texture using Perlin-style noise."""
    # Multi-octave noise for realistic texture
    texture = np.zeros((size, size), dtype=np.float32)

    for octave in range(octaves):
        freq = 2 ** octave
        amplitude = 0.5 ** octave

        # Generate random noise at this frequency
        noise_size = size // freq + 1
        noise = np.random.rand(noise_size, noise_size)

        # Upsample with bilinear interpolation
        from scipy.ndimage import zoom
        upsampled = zoom(noise, freq, order=1)[:size, :size]

        texture += upsampled * amplitude

    # Normalize and add fine detail
    texture = (texture - texture.min()) / (texture.max() - texture.min())

    # Add very fine grain
    fine_noise = np.random.normal(0, 0.02, (size, size))
    texture = np.clip(texture + fine_noise, 0, 1)

    return texture


def create_circular_mask(size, center, radius):
    """Create circular alpha mask."""
    y, x = np.ogrid[:size, :size]
    dist = np.sqrt((x - center)**2 + (y - center)**2)
    mask = (dist <= radius).astype(np.float32)

    # Smooth edge
    edge_width = 2
    edge_mask = (dist > radius - edge_width) & (dist <= radius)
    edge_fade = 1.0 - (dist[edge_mask] - (radius - edge_width)) / edge_width
    mask[edge_mask] = edge_fade

    return mask


def generate_albedo():
    """Generate base color with fine texture."""
    print("Generating albedo (fine-grain texture)...")

    # Base dark color with slight warmth (like carbon/leather)
    base_color = np.array([0.18, 0.18, 0.20])  # Dark gray with blue tint

    # Create fine grain texture
    texture = create_fine_grain_texture(SIZE, scale=0.3, octaves=5)

    # Modulate brightness with texture
    rgb = np.zeros((SIZE, SIZE, 3), dtype=np.float32)
    for i in range(3):
        rgb[:, :, i] = base_color[i] * (0.7 + texture * 0.3)

    # Add subtle radial darkening towards center
    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    radial = 1.0 - (dist / RADIUS) * 0.2
    radial = np.clip(radial, 0.8, 1.0)

    for i in range(3):
        rgb[:, :, i] *= radial

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * 255).astype(np.uint8)

    # Convert to 8-bit
    rgb = np.clip(rgb * 255, 0, 255).astype(np.uint8)

    # Create RGBA image
    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata(list(zip(rgb[:, :, 0].flatten(),
                         rgb[:, :, 1].flatten(),
                         rgb[:, :, 2].flatten(),
                         alpha.flatten())))

    return img


def generate_tick_marks():
    """Generate circular tick marks around outer ring."""
    print("Generating tick marks...")

    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Ring parameters
    ring_radius = RADIUS - 35
    tick_count = 48  # Number of tick marks
    major_tick_every = 6  # Major tick every N ticks

    for i in range(tick_count):
        angle = (i / tick_count) * 2 * np.pi - np.pi / 2

        # Major vs minor ticks
        is_major = (i % major_tick_every) == 0
        tick_length = 15 if is_major else 8
        tick_width = 2 if is_major else 1

        # Inner and outer points
        inner_r = ring_radius - tick_length
        outer_r = ring_radius

        x1 = CENTER + inner_r * np.cos(angle)
        y1 = CENTER + inner_r * np.sin(angle)
        x2 = CENTER + outer_r * np.cos(angle)
        y2 = CENTER + outer_r * np.sin(angle)

        # Draw tick mark (light gray)
        color = (200, 200, 210, 255) if is_major else (160, 160, 170, 200)
        draw.line([(x1, y1), (x2, y2)], fill=color, width=tick_width)

    return img


def generate_bezel():
    """Generate metallic bezel rim."""
    print("Generating bezel rim...")

    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Outer bezel ring (metallic)
    bezel_outer = RADIUS + 18
    bezel_inner = RADIUS - 2

    # Create gradient for metallic look
    bezel_data = np.zeros((SIZE, SIZE, 4), dtype=np.uint8)

    y, x = np.meshgrid(np.arange(SIZE), np.arange(SIZE), indexing='ij')
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    in_bezel = (dist >= bezel_inner) & (dist <= bezel_outer)

    if np.any(in_bezel):
        # Metallic gradient (dark to light to dark)
        normalized = (dist[in_bezel] - bezel_inner) / (bezel_outer - bezel_inner)

        # Create brushed metal effect
        metal_brightness = 0.4 + 0.3 * np.sin(normalized * np.pi)

        # Add anisotropic variation
        angles = np.arctan2(y - CENTER, x - CENTER)
        aniso_variation = 0.1 * np.sin(angles[in_bezel] * 20)  # Fine radial lines

        brightness = np.clip(metal_brightness + aniso_variation, 0, 1)

        # Metallic color (cool gray)
        bezel_data[in_bezel, 0] = (brightness * 180).astype(np.uint8)  # R
        bezel_data[in_bezel, 1] = (brightness * 185).astype(np.uint8)  # G
        bezel_data[in_bezel, 2] = (brightness * 190).astype(np.uint8)  # B
        bezel_data[in_bezel, 3] = 255  # Alpha

    img = Image.fromarray(bezel_data, 'RGBA')

    # Add subtle blur for smoothness
    img = img.filter(ImageFilter.GaussianBlur(radius=0.5))

    return img


def generate_ao():
    """Generate ambient occlusion."""
    print("Generating ambient occlusion...")

    # Dark around edges and in tick mark ring
    ao = np.ones((SIZE, SIZE), dtype=np.float32)

    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    # Edge darkening
    edge_ao = create_circular_gradient(SIZE, CENTER, RADIUS - 30, RADIUS, invert=True)
    ao *= (1.0 - edge_ao * 0.6)

    # Ring area darkening (where tick marks are)
    ring_radius = RADIUS - 35
    ring_ao = np.abs(dist - ring_radius) < 20
    ao[ring_ao] *= 0.6

    # Center slight darkening
    center_mask = dist < 100
    ao[center_mask] *= 0.9

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * 255).astype(np.uint8)

    # Convert to grayscale with alpha
    ao_uint8 = (ao * 255).astype(np.uint8)

    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata(list(zip(ao_uint8.flatten(),
                         ao_uint8.flatten(),
                         ao_uint8.flatten(),
                         alpha.flatten())))

    return img


def generate_roughness():
    """Generate roughness map."""
    print("Generating roughness...")

    # Most of surface is fairly rough (textured plastic/rubber)
    roughness = np.ones((SIZE, SIZE), dtype=np.float32) * 0.7

    # Add texture variation
    texture = create_fine_grain_texture(SIZE, scale=0.4, octaves=3)
    roughness += (texture - 0.5) * 0.3
    roughness = np.clip(roughness, 0.5, 0.9)

    # Ring area slightly smoother (worn from use)
    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)
    ring_radius = RADIUS - 35
    ring_mask = np.abs(dist - ring_radius) < 25
    roughness[ring_mask] *= 0.85

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * 255).astype(np.uint8)

    # Convert to grayscale with alpha
    roughness_uint8 = (roughness * 255).astype(np.uint8)

    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata(list(zip(roughness_uint8.flatten(),
                         roughness_uint8.flatten(),
                         roughness_uint8.flatten(),
                         alpha.flatten())))

    return img


def generate_specular():
    """Generate specular highlights."""
    print("Generating specular highlights...")

    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))

    # Create subtle rim lighting
    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    # Top-left rim light
    light_angle = np.arctan2(y - CENTER, x - CENTER)
    light_direction = -np.pi * 0.75  # Top-left
    angle_diff = np.abs(light_angle - light_direction)
    angle_diff = np.minimum(angle_diff, 2 * np.pi - angle_diff)

    # Highlight near edge
    edge_highlight = (dist > RADIUS - 40) & (dist < RADIUS - 10)
    angle_mask = angle_diff < np.pi / 3

    specular = np.zeros((SIZE, SIZE), dtype=np.float32)
    highlight_mask = edge_highlight & angle_mask

    if np.any(highlight_mask):
        intensity = (1.0 - angle_diff[highlight_mask] / (np.pi / 3)) * 0.5
        edge_factor = 1.0 - np.abs(dist[highlight_mask] - (RADIUS - 25)) / 15
        edge_factor = np.clip(edge_factor, 0, 1)
        specular[highlight_mask] = intensity * edge_factor

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * 255).astype(np.uint8)

    # Convert to 8-bit
    specular_uint8 = (specular * 255).astype(np.uint8)

    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata(list(zip(specular_uint8.flatten(),
                         specular_uint8.flatten(),
                         specular_uint8.flatten(),
                         alpha.flatten())))

    return img


def generate_contact_shadow():
    """Generate contact shadow at base."""
    print("Generating contact shadow...")

    # Soft shadow around bottom edge
    shadow = np.zeros((SIZE, SIZE), dtype=np.float32)

    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    # Shadow gradient from outer edge inward
    shadow_inner = RADIUS - 25
    shadow_outer = RADIUS + 15

    in_shadow = (dist >= shadow_inner) & (dist <= shadow_outer)

    if np.any(in_shadow):
        # Stronger at bottom
        angle = np.arctan2(y - CENTER, x - CENTER)
        bottom_bias = 0.5 + 0.5 * np.sin(angle)  # Stronger at bottom

        # Radial falloff
        radial = 1.0 - (dist[in_shadow] - shadow_inner) / (shadow_outer - shadow_inner)

        shadow[in_shadow] = radial * bottom_bias[in_shadow] * 0.8

    # Apply circular mask (slightly larger)
    mask = create_circular_mask(SIZE, CENTER, RADIUS + 20)
    alpha = (mask * shadow * 255).astype(np.uint8)

    # Black shadow with variable alpha
    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata([(0, 0, 0, a) for a in alpha.flatten()])

    # Blur for softness
    img = img.filter(ImageFilter.GaussianBlur(radius=8))

    return img


def generate_indicator():
    """Generate position indicator dot."""
    print("Generating indicator...")

    img = Image.new('RGBA', (SIZE, SIZE), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)

    # Small dot at top
    dot_radius = 4
    dot_distance = RADIUS - 50
    dot_x = CENTER
    dot_y = CENTER - dot_distance

    # Draw glowing dot
    for i in range(3, 0, -1):
        alpha = int(255 * (1.0 / i))
        color = (255, 255, 255, alpha)
        draw.ellipse([dot_x - dot_radius * i, dot_y - dot_radius * i,
                     dot_x + dot_radius * i, dot_y + dot_radius * i],
                    fill=color)

    return img


def generate_glow():
    """Generate subtle glow effect."""
    print("Generating glow...")

    # Very subtle inner glow
    glow = np.zeros((SIZE, SIZE), dtype=np.float32)

    y, x = np.ogrid[:SIZE, :SIZE]
    dist = np.sqrt((x - CENTER)**2 + (y - CENTER)**2)

    # Soft center glow
    center_glow = np.exp(-(dist / 150)**2) * 0.15
    glow += center_glow

    # Ring glow around tick marks
    ring_radius = RADIUS - 35
    ring_dist = np.abs(dist - ring_radius)
    ring_glow = np.exp(-(ring_dist / 10)**2) * 0.1
    glow += ring_glow

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * glow * 255).astype(np.uint8)

    # Warm white glow (slight blue tint)
    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata([(220, 230, 255, a) for a in alpha.flatten()])

    # Heavy blur for glow
    img = img.filter(ImageFilter.GaussianBlur(radius=15))

    return img


def generate_normal_map():
    """Generate normal map for surface detail (optional)."""
    print("Generating normal map...")

    # Create height map from texture
    texture = create_fine_grain_texture(SIZE, scale=0.3, octaves=5)

    # Sobel filter for normal calculation
    from scipy.ndimage import sobel

    grad_x = sobel(texture, axis=1) * 0.1
    grad_y = sobel(texture, axis=0) * 0.1

    # Normal map: encode XY gradients in RG, Z always pointing up
    normal_x = (grad_x + 1.0) * 0.5  # Remap to 0-1
    normal_y = (grad_y + 1.0) * 0.5
    normal_z = np.ones_like(texture) * 0.5 + 0.5  # Mostly pointing up

    # Apply circular mask
    mask = create_circular_mask(SIZE, CENTER, RADIUS)
    alpha = (mask * 255).astype(np.uint8)

    # Convert to 8-bit
    normal_x_uint8 = (normal_x * 255).astype(np.uint8)
    normal_y_uint8 = (normal_y * 255).astype(np.uint8)
    normal_z_uint8 = (normal_z * 255).astype(np.uint8)

    img = Image.new('RGBA', (SIZE, SIZE))
    img.putdata(list(zip(normal_x_uint8.flatten(),
                         normal_y_uint8.flatten(),
                         normal_z_uint8.flatten(),
                         alpha.flatten())))

    return img


def generate_manifest():
    """Generate manifest.json for the asset pack."""
    manifest = {
        "logicalSize": SIZE,
        "pivot": {"x": 0.5, "y": 0.5},
        "layers": [
            {
                "name": "contact_shadow",
                "file": "contact_shadow.png",
                "blend": "multiply",
                "opacity": 0.8,
                "rotates": False
            },
            {
                "name": "albedo",
                "file": "albedo.png",
                "blend": "normal",
                "opacity": 1.0,
                "rotates": True
            },
            {
                "name": "bezel",
                "file": "bezel.png",
                "blend": "normal",
                "opacity": 1.0,
                "rotates": False
            },
            {
                "name": "tick_marks",
                "file": "tick_marks.png",
                "blend": "screen",
                "opacity": 0.9,
                "rotates": False
            },
            {
                "name": "ao",
                "file": "ao.png",
                "blend": "multiply",
                "opacity": 0.7,
                "rotates": False
            },
            {
                "name": "roughness",
                "file": "roughness.png",
                "blend": "multiply",
                "opacity": 0.3,
                "rotates": False
            },
            {
                "name": "specular",
                "file": "specular.png",
                "blend": "screen",
                "opacity": 0.6,
                "rotates": False
            },
            {
                "name": "glow",
                "file": "glow.png",
                "blend": "add",
                "opacity": 0.3,
                "rotates": False,
                "glow": True
            },
            {
                "name": "indicator",
                "file": "indicator.png",
                "blend": "add",
                "opacity": 1.0,
                "rotates": True,
                "indicator": True
            }
        ]
    }

    return manifest


def main():
    """Generate all PBR layers and manifest."""
    output_dir = "assets/knob_industrial"
    os.makedirs(output_dir, exist_ok=True)

    print(f"\n{'='*60}")
    print("Procedural Industrial Knob PBR Generator")
    print(f"{'='*60}\n")
    print(f"Output directory: {output_dir}/")
    print(f"Resolution: {SIZE}x{SIZE}")
    print()

    # Generate all layers
    layers = {
        'albedo.png': generate_albedo(),
        'tick_marks.png': generate_tick_marks(),
        'bezel.png': generate_bezel(),
        'ao.png': generate_ao(),
        'roughness.png': generate_roughness(),
        'specular.png': generate_specular(),
        'contact_shadow.png': generate_contact_shadow(),
        'indicator.png': generate_indicator(),
        'glow.png': generate_glow(),
        'normal.png': generate_normal_map(),  # Optional, not in manifest
    }

    # Save all layers
    print("\nSaving layers...")
    for filename, img in layers.items():
        filepath = os.path.join(output_dir, filename)
        img.save(filepath)
        size_kb = os.path.getsize(filepath) / 1024
        print(f"  ✓ {filename:20} ({size_kb:6.1f} KB)")

    # Generate and save manifest
    print("\nGenerating manifest...")
    manifest = generate_manifest()
    manifest_path = os.path.join(output_dir, 'manifest.json')
    with open(manifest_path, 'w') as f:
        json.dump(manifest, f, indent=2)
    print(f"  ✓ manifest.json")

    # Summary
    total_size = sum(os.path.getsize(os.path.join(output_dir, f))
                    for f in os.listdir(output_dir))

    print(f"\n{'='*60}")
    print("Generation complete!")
    print(f"{'='*60}")
    print(f"Total layers: {len(layers)}")
    print(f"Total size: {total_size / 1024:.1f} KB")
    print(f"\nAssets saved to: {output_dir}/")
    print("\nNext steps:")
    print("1. Add 'knob_industrial' to MainComponent pack list")
    print("2. Rebuild MonumentPlayground")
    print("3. Use ← → keys to switch to new pack")
    print()


if __name__ == "__main__":
    # Check dependencies
    try:
        import scipy.ndimage
    except ImportError:
        print("Error: scipy not found. Install with: pip3 install scipy")
        exit(1)

    main()