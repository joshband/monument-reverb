#!/usr/bin/env python3
"""Preview enhanced layered knob composite with environmental effects.

Composites all layers (base, ring, indicator, cap) plus FX (shadows, AO, reflections)
at various rotation angles to verify alignment and visual quality.

Usage:
    # Static sequence (original)
    python3 preview_knob_composite_enhanced.py --material granite

    # Animated GIF (auto-looping)
    python3 preview_knob_composite_enhanced.py --material granite --animated

    # Interactive HTML viewer (slider control)
    python3 preview_knob_composite_enhanced.py --material granite --interactive

    # Single rotation
    python3 preview_knob_composite_enhanced.py --material marble --rotation 45
"""

import sys
import argparse
import json
from pathlib import Path

try:
    from PIL import Image
    import math
except ImportError:
    print("❌ PIL/Pillow not found!")
    print("Install with: pip3 install Pillow")
    sys.exit(1)


def rotate_image(image, angle_degrees):
    """Rotate image around center with high-quality resampling."""
    return image.rotate(-angle_degrees, resample=Image.BICUBIC, expand=False)


def composite_knob(base_dir, material, rotation_degrees=0):
    """Composite all knob layers at a given rotation angle (8 layers + FX)."""
    base_path = Path(base_dir)

    if not base_path.exists():
        print(f"❌ Base directory not found: {base_path}")
        return None

    # Find layer files (new 8-layer structure - all in root directory)
    layer_files = {
        'shadow': base_path / f"fx_shadow_{material}.png",  # FX layer (multiply blend)
        'panel_bezel': base_path / "layer_0_panel_bezel.png",  # Layer 0 (static)
        'base': base_path / f"layer_1_base_{material}.png",  # Layer 1 (rotates)
        'ring': base_path / f"layer_2_ring_{material}.png",  # Layer 2 (static)
        'indicator': None,  # Layer 3 (rotates) - will be detected
        'cap': None,  # Layer 4 (static) - will be detected
        'led_ring': base_path / "layer_5_led_ring.png",  # Layer 5 (static)
        'led_bloom': base_path / "layer_6_led_bloom.png",  # Layer 6 (static, additive)
        'text_label': base_path / "layer_7_text_label.png"  # Layer 7 (static)
    }

    # Detect indicator and cap (may have different types)
    for f in base_path.glob("layer_3_indicator_*.png"):
        layer_files['indicator'] = f
        break

    for f in base_path.glob("layer_4_cap_*.png"):
        layer_files['cap'] = f
        break

    # Load layers
    layers = {}
    for name, path in layer_files.items():
        if path is None:
            print(f"⚠️  Skipping {name} layer (not found)")
            continue

        if not path.exists():
            print(f"⚠️  Layer not found: {path}")
            continue

        try:
            img = Image.open(path).convert("RGBA")
            layers[name] = img
            print(f"✅ Loaded {name}: {path.name} ({img.size[0]}x{img.size[1]})")
        except Exception as e:
            print(f"❌ Failed to load {name}: {e}")

    if not layers:
        print("❌ No layers loaded!")
        return None

    # Get canvas size from first layer
    canvas_size = layers[list(layers.keys())[0]].size

    # Create composite canvas (RGBA)
    composite = Image.new("RGBA", canvas_size, (0, 0, 0, 0))

    # Layer 0: Shadow (multiply blend, static)
    if 'shadow' in layers:
        # Multiply blend simulation: darken only
        shadow_layer = layers['shadow']
        composite = Image.alpha_composite(composite, shadow_layer)
        print(f"   🌑 Applied shadow (static, multiply blend)")

    # Layer 1: Panel bezel (static, background frame)
    if 'panel_bezel' in layers:
        composite = Image.alpha_composite(composite, layers['panel_bezel'])
        print(f"   🔲 Applied panel bezel (static)")

    # Layer 2: Base body (rotates)
    if 'base' in layers:
        base_rotated = rotate_image(layers['base'], rotation_degrees)
        composite = Image.alpha_composite(composite, base_rotated)
        print(f"   🔄 Applied base (rotated {rotation_degrees}°)")

    # Layer 3: Detail ring (static)
    if 'ring' in layers:
        composite = Image.alpha_composite(composite, layers['ring'])
        print(f"   ⭕ Applied ring (static)")

    # Layer 4: Indicator (rotates)
    if 'indicator' in layers:
        indicator_rotated = rotate_image(layers['indicator'], rotation_degrees)
        composite = Image.alpha_composite(composite, indicator_rotated)
        print(f"   🔄 Applied indicator (rotated {rotation_degrees}°)")

    # Layer 5: Center cap (static)
    if 'cap' in layers:
        composite = Image.alpha_composite(composite, layers['cap'])
        print(f"   ⚪ Applied cap (static)")

    # Layer 6: LED ring (static, emission glow)
    if 'led_ring' in layers:
        composite = Image.alpha_composite(composite, layers['led_ring'])
        print(f"   💡 Applied LED ring (static)")

    # Layer 7: LED bloom (static, additive blend)
    if 'led_bloom' in layers:
        # Additive blend: add RGB values (lighten only)
        from PIL import ImageChops
        bloom_layer = layers['led_bloom']
        # Convert to RGB for additive blend, then back to RGBA
        comp_rgb = composite.convert("RGB")
        bloom_rgb = bloom_layer.convert("RGB")
        blended = ImageChops.add(comp_rgb, bloom_rgb, scale=2.0, offset=0)
        blended_rgba = blended.convert("RGBA")
        # Use bloom's alpha as mask
        composite = Image.composite(blended_rgba, composite, bloom_layer.split()[3])
        print(f"   ✨ Applied LED bloom (static, additive blend)")

    # Layer 8: Text label (static)
    if 'text_label' in layers:
        composite = Image.alpha_composite(composite, layers['text_label'])
        print(f"   📝 Applied text label (static)")

    return composite


def create_rotation_sequence(base_dir, material, output_path):
    """Create a strip showing knob at multiple rotation angles."""
    rotations = [0, 45, 90, 135, 180, 225, 270, 315]
    composites = []

    print(f"\nGenerating rotation sequence for {material}...")

    for rot in rotations:
        print(f"\n▶ Rotation: {rot}°")
        comp = composite_knob(base_dir, material, rotation_degrees=rot)
        if comp:
            composites.append(comp)

    if not composites:
        print("❌ No composites generated!")
        return

    # Create horizontal strip
    width, height = composites[0].size
    strip = Image.new("RGBA", (width * len(composites), height), (30, 30, 35, 255))

    for i, comp in enumerate(composites):
        strip.paste(comp, (i * width, 0), comp)

    # Save
    strip.save(output_path)
    print(f"\n✅ Saved rotation sequence: {output_path}")
    print(f"   ({width * len(composites)}x{height} pixels)")


def create_animated_gif(base_dir, material, output_path, fps=15):
    """Create an animated GIF showing smooth knob rotation."""
    # Generate more frames for smoother animation (36 frames = 10° increments)
    num_frames = 36
    rotations = [i * (360 / num_frames) for i in range(num_frames)]
    frames = []

    print(f"\n🎬 Generating animated GIF for {material}...")
    print(f"   Frames: {num_frames} ({360/num_frames}° increments)")
    print(f"   FPS: {fps}")

    for i, rot in enumerate(rotations):
        print(f"   Frame {i+1}/{num_frames}: {rot:.1f}°", end='\r')
        comp = composite_knob(base_dir, material, rotation_degrees=rot)
        if comp:
            # Convert RGBA to RGB with background for GIF compatibility
            bg = Image.new("RGB", comp.size, (30, 30, 35))
            bg.paste(comp, (0, 0), comp)
            frames.append(bg)

    print()  # New line after progress

    if not frames:
        print("❌ No frames generated!")
        return

    # Save as animated GIF
    duration = int(1000 / fps)  # Duration per frame in ms
    frames[0].save(
        output_path,
        save_all=True,
        append_images=frames[1:],
        duration=duration,
        loop=0,  # Infinite loop
        optimize=True
    )

    print(f"\n✅ Saved animated GIF: {output_path}")
    print(f"   Size: {frames[0].size[0]}x{frames[0].size[1]}")
    print(f"   Frames: {len(frames)} @ {fps} FPS")
    print(f"   Duration: {len(frames)/fps:.1f}s per loop")


def create_interactive_html(base_dir, material, output_path):
    """Create an interactive HTML viewer with rotation slider."""
    import base64
    import io

    print(f"\n🌐 Generating interactive HTML viewer for {material}...")

    # Generate frames at 10° increments (36 frames)
    num_frames = 36
    rotations = [i * (360 / num_frames) for i in range(num_frames)]
    frames_b64 = []

    for i, rot in enumerate(rotations):
        print(f"   Frame {i+1}/{num_frames}: {rot:.1f}°", end='\r')
        comp = composite_knob(base_dir, material, rotation_degrees=rot)
        if comp:
            # Convert to base64 PNG for embedding
            buffer = io.BytesIO()
            comp.save(buffer, format="PNG")
            b64 = base64.b64encode(buffer.getvalue()).decode('utf-8')
            frames_b64.append(b64)

    print()  # New line after progress

    if not frames_b64:
        print("❌ No frames generated!")
        return

    # Generate HTML with embedded images
    html = f"""<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <title>{material.capitalize()} Knob - Interactive Preview</title>
    <style>
        body {{
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Arial, sans-serif;
            background: linear-gradient(135deg, #1e1e20 0%, #2d2d30 100%);
            color: #e0e0e0;
            margin: 0;
            padding: 20px;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
            min-height: 100vh;
        }}

        h1 {{
            margin: 0 0 10px 0;
            font-size: 32px;
            font-weight: 300;
            text-align: center;
        }}

        .subtitle {{
            color: #888;
            margin-bottom: 30px;
            font-size: 14px;
        }}

        .container {{
            background: rgba(0, 0, 0, 0.3);
            border-radius: 16px;
            padding: 40px;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.4);
            backdrop-filter: blur(10px);
        }}

        #knob-canvas {{
            display: block;
            margin: 0 auto 30px;
            border-radius: 8px;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5);
        }}

        .controls {{
            max-width: 600px;
            margin: 0 auto;
        }}

        .slider-container {{
            margin-bottom: 20px;
        }}

        input[type="range"] {{
            width: 100%;
            height: 8px;
            border-radius: 4px;
            background: linear-gradient(to right, #4a9eff 0%, #4a9eff var(--value), #333 var(--value), #333 100%);
            outline: none;
            -webkit-appearance: none;
        }}

        input[type="range"]::-webkit-slider-thumb {{
            -webkit-appearance: none;
            appearance: none;
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #4a9eff;
            cursor: pointer;
            box-shadow: 0 2px 8px rgba(74, 158, 255, 0.5);
        }}

        input[type="range"]::-moz-range-thumb {{
            width: 24px;
            height: 24px;
            border-radius: 50%;
            background: #4a9eff;
            cursor: pointer;
            border: none;
            box-shadow: 0 2px 8px rgba(74, 158, 255, 0.5);
        }}

        .value-display {{
            text-align: center;
            font-size: 48px;
            font-weight: 200;
            margin-bottom: 10px;
            font-variant-numeric: tabular-nums;
            letter-spacing: -0.02em;
        }}

        .label {{
            text-align: center;
            color: #888;
            font-size: 12px;
            text-transform: uppercase;
            letter-spacing: 0.1em;
            margin-bottom: 15px;
        }}

        .buttons {{
            display: flex;
            gap: 10px;
            justify-content: center;
            margin-top: 20px;
        }}

        button {{
            background: #4a9eff;
            color: white;
            border: none;
            padding: 12px 24px;
            border-radius: 8px;
            cursor: pointer;
            font-size: 14px;
            font-weight: 500;
            transition: all 0.2s;
        }}

        button:hover {{
            background: #3a8eef;
            transform: translateY(-1px);
            box-shadow: 0 4px 12px rgba(74, 158, 255, 0.3);
        }}

        button:active {{
            transform: translateY(0);
        }}

        button:disabled {{
            background: #333;
            color: #666;
            cursor: not-allowed;
            transform: none;
        }}

        .info {{
            text-align: center;
            margin-top: 30px;
            color: #666;
            font-size: 12px;
        }}

        .background-presets {{
            display: flex;
            gap: 8px;
            justify-content: center;
            flex-wrap: wrap;
            margin-bottom: 15px;
        }}

        .bg-preset {{
            background: #333;
            color: #aaa;
            border: 2px solid #444;
            padding: 8px 16px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 12px;
            font-weight: 500;
            transition: all 0.2s;
        }}

        .bg-preset:hover {{
            background: #3a3a3a;
            border-color: #555;
            color: #ccc;
        }}

        .bg-preset.active {{
            background: #4a9eff;
            border-color: #4a9eff;
            color: white;
        }}

        .file-upload {{
            text-align: center;
            margin-top: 10px;
        }}

        .upload-label {{
            display: inline-block;
            background: #333;
            color: #aaa;
            border: 2px dashed #555;
            padding: 10px 20px;
            border-radius: 6px;
            cursor: pointer;
            font-size: 12px;
            transition: all 0.2s;
        }}

        .upload-label:hover {{
            background: #3a3a3a;
            border-color: #666;
            color: #ccc;
        }}
    </style>
</head>
<body>
    <h1>🎨 {material.capitalize()} Knob</h1>
    <div class="subtitle">Interactive Preview - Enhanced Layered System</div>

    <div class="container">
        <canvas id="knob-canvas" width="512" height="512"></canvas>

        <div class="controls">
            <div class="value-display" id="angle-display">0°</div>
            <div class="label">Rotation Angle</div>

            <div class="slider-container">
                <input type="range" id="rotation-slider" min="0" max="{num_frames-1}" value="0" step="1">
            </div>

            <div class="buttons">
                <button id="animate-btn">▶ Animate</button>
                <button id="reset-btn">↺ Reset</button>
            </div>

            <div class="label" style="margin-top: 30px;">Background Panel</div>

            <div class="background-presets">
                <button class="bg-preset" data-bg="transparent">Transparent</button>
                <button class="bg-preset active" data-bg="dark">Dark Gray</button>
                <button class="bg-preset" data-bg="light">Light Gray</button>
                <button class="bg-preset" data-bg="wood">Wood</button>
                <button class="bg-preset" data-bg="metal">Brushed Metal</button>
            </div>

            <div class="file-upload">
                <label for="bg-upload" class="upload-label">
                    📂 Upload Custom Background
                    <input type="file" id="bg-upload" accept="image/*" style="display: none;">
                </label>
            </div>
        </div>

        <div class="info">
            Drag slider or click Animate • {num_frames} frames • Monument Reverb
        </div>
    </div>

    <script>
        const canvas = document.getElementById('knob-canvas');
        const ctx = canvas.getContext('2d');
        const slider = document.getElementById('rotation-slider');
        const angleDisplay = document.getElementById('angle-display');
        const animateBtn = document.getElementById('animate-btn');
        const resetBtn = document.getElementById('reset-btn');
        const bgUpload = document.getElementById('bg-upload');
        const bgPresetBtns = document.querySelectorAll('.bg-preset');

        // Preload all frames
        const frames = [];
        const frameData = {json.dumps([f'data:image/png;base64,{b64}' for b64 in frames_b64])};

        let isAnimating = false;
        let animationFrame = null;
        let currentBackground = 'dark';
        let customBackgroundImage = null;

        // Load all images
        console.log('Loading {len(frames_b64)} frames...');
        Promise.all(frameData.map((src, i) => {{
            return new Promise((resolve) => {{
                const img = new Image();
                img.onload = () => {{
                    frames[i] = img;
                    resolve();
                }};
                img.src = src;
            }});
        }})).then(() => {{
            console.log('All frames loaded!');
            updateFrame(0);
        }});

        // Background generators
        function drawBackground() {{
            if (currentBackground === 'transparent') {{
                ctx.clearRect(0, 0, canvas.width, canvas.height);
            }} else if (currentBackground === 'dark') {{
                ctx.fillStyle = '#2a2a2d';
                ctx.fillRect(0, 0, canvas.width, canvas.height);
            }} else if (currentBackground === 'light') {{
                ctx.fillStyle = '#e0e0e2';
                ctx.fillRect(0, 0, canvas.width, canvas.height);
            }} else if (currentBackground === 'wood') {{
                drawWoodTexture();
            }} else if (currentBackground === 'metal') {{
                drawBrushedMetal();
            }} else if (currentBackground === 'custom' && customBackgroundImage) {{
                ctx.drawImage(customBackgroundImage, 0, 0, canvas.width, canvas.height);
            }}
        }}

        function drawWoodTexture() {{
            // Create wood grain effect
            const gradient = ctx.createLinearGradient(0, 0, canvas.width, 0);
            gradient.addColorStop(0, '#3d2817');
            gradient.addColorStop(0.5, '#5a3d2b');
            gradient.addColorStop(1, '#3d2817');
            ctx.fillStyle = gradient;
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            // Add grain lines
            ctx.strokeStyle = 'rgba(0, 0, 0, 0.1)';
            ctx.lineWidth = 1;
            for (let i = 0; i < 50; i++) {{
                const x = Math.random() * canvas.width;
                const y = Math.random() * canvas.height;
                const length = 50 + Math.random() * 100;
                ctx.beginPath();
                ctx.moveTo(x, y);
                ctx.lineTo(x + length, y);
                ctx.stroke();
            }}
        }}

        function drawBrushedMetal() {{
            // Create brushed aluminum effect
            ctx.fillStyle = '#a8a8aa';
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            // Add vertical brushed lines
            ctx.strokeStyle = 'rgba(255, 255, 255, 0.05)';
            ctx.lineWidth = 0.5;
            for (let x = 0; x < canvas.width; x += 2) {{
                const alpha = 0.02 + Math.random() * 0.03;
                ctx.strokeStyle = `rgba(255, 255, 255, ${{alpha}})`;
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }}

            // Add darker vertical lines for contrast
            ctx.strokeStyle = 'rgba(0, 0, 0, 0.03)';
            for (let x = 1; x < canvas.width; x += 4) {{
                ctx.beginPath();
                ctx.moveTo(x, 0);
                ctx.lineTo(x, canvas.height);
                ctx.stroke();
            }}
        }}

        function updateFrame(index) {{
            // Draw background first
            drawBackground();

            // Draw knob on top
            if (frames[index]) {{
                ctx.drawImage(frames[index], 0, 0);
            }}

            const angle = Math.round(index * (360 / {num_frames}));
            angleDisplay.textContent = angle + '°';
            slider.value = index;

            // Update slider gradient
            const percent = (index / ({num_frames-1})) * 100;
            slider.style.setProperty('--value', percent + '%');
        }}

        function animate() {{
            if (isAnimating) {{
                // Stop animation
                isAnimating = false;
                animateBtn.textContent = '▶ Animate';
                if (animationFrame) {{
                    cancelAnimationFrame(animationFrame);
                }}
            }} else {{
                // Start animation
                isAnimating = true;
                animateBtn.textContent = '⏸ Pause';

                let currentFrame = parseInt(slider.value);
                const fps = 30;
                const frameTime = 1000 / fps;
                let lastTime = performance.now();

                function step(currentTime) {{
                    if (!isAnimating) return;

                    const deltaTime = currentTime - lastTime;

                    if (deltaTime >= frameTime) {{
                        currentFrame = (currentFrame + 1) % {num_frames};
                        updateFrame(currentFrame);
                        lastTime = currentTime;
                    }}

                    animationFrame = requestAnimationFrame(step);
                }}

                animationFrame = requestAnimationFrame(step);
            }}
        }}

        // Event listeners
        slider.addEventListener('input', (e) => {{
            if (isAnimating) {{
                animate(); // Stop animation if user interacts
            }}
            updateFrame(parseInt(e.target.value));
        }});

        animateBtn.addEventListener('click', animate);

        resetBtn.addEventListener('click', () => {{
            if (isAnimating) {{
                animate(); // Stop if animating
            }}
            updateFrame(0);
        }});

        // Keyboard controls
        document.addEventListener('keydown', (e) => {{
            const currentIndex = parseInt(slider.value);

            if (e.key === 'ArrowLeft') {{
                e.preventDefault();
                updateFrame(Math.max(0, currentIndex - 1));
            }} else if (e.key === 'ArrowRight') {{
                e.preventDefault();
                updateFrame(Math.min({num_frames-1}, currentIndex + 1));
            }} else if (e.key === ' ') {{
                e.preventDefault();
                animate();
            }} else if (e.key === 'r' || e.key === 'R') {{
                e.preventDefault();
                if (isAnimating) animate();
                updateFrame(0);
            }}
        }});

        // Background preset buttons
        bgPresetBtns.forEach(btn => {{
            btn.addEventListener('click', () => {{
                // Update active state
                bgPresetBtns.forEach(b => b.classList.remove('active'));
                btn.classList.add('active');

                // Update background
                currentBackground = btn.dataset.bg;
                updateFrame(parseInt(slider.value));
            }});
        }});

        // Custom background upload
        bgUpload.addEventListener('change', (e) => {{
            const file = e.target.files[0];
            if (file && file.type.startsWith('image/')) {{
                const reader = new FileReader();
                reader.onload = (event) => {{
                    const img = new Image();
                    img.onload = () => {{
                        customBackgroundImage = img;
                        currentBackground = 'custom';

                        // Update button states
                        bgPresetBtns.forEach(b => b.classList.remove('active'));

                        // Redraw with custom background
                        updateFrame(parseInt(slider.value));
                    }};
                    img.src = event.target.result;
                }};
                reader.readAsDataURL(file);
            }}
        }});
    </script>
</body>
</html>"""

    # Save HTML file
    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(html)

    print(f"\n✅ Saved interactive HTML: {output_path}")
    print(f"   Frames: {len(frames_b64)}")
    print(f"   Controls: Slider, Animate button, Arrow keys, Spacebar")


def main():
    parser = argparse.ArgumentParser(
        description="Preview enhanced knob composite with environmental effects"
    )
    parser.add_argument(
        "--material",
        default="granite",
        help="Material name (granite, marble, basalt, etc.)"
    )
    parser.add_argument(
        "--rotation",
        type=float,
        default=None,
        help="Rotation angle in degrees (default: create sequence)"
    )
    parser.add_argument(
        "--animated",
        action="store_true",
        help="Create animated GIF (auto-looping)"
    )
    parser.add_argument(
        "--interactive",
        action="store_true",
        help="Create interactive HTML viewer with slider"
    )
    parser.add_argument(
        "--fps",
        type=int,
        default=15,
        help="Frames per second for animated GIF (default: 15)"
    )
    parser.add_argument(
        "--base-dir",
        default="assets/ui/knobs_enhanced",
        help="Base directory containing material subdirectories"
    )
    parser.add_argument(
        "--out",
        help="Output file path"
    )

    args = parser.parse_args()

    print("=" * 70)
    print("🎨 Enhanced Knob Composite Preview")
    print("=" * 70)
    print(f"Material: {args.material}")
    print(f"Base dir: {args.base_dir}")

    if args.animated:
        # Animated GIF mode
        if args.out:
            output_path = Path(args.out)
        else:
            output_path = Path(f"/tmp/knob_{args.material}_animated.gif")

        create_animated_gif(args.base_dir, args.material, output_path, args.fps)

        # Try to open
        import subprocess
        try:
            if sys.platform == "darwin":
                subprocess.run(["open", str(output_path)], check=False)
                print("   (Opened in system viewer)")
            elif sys.platform == "linux":
                subprocess.run(["xdg-open", str(output_path)], check=False)
                print("   (Opened in system viewer)")
        except:
            pass

    elif args.interactive:
        # Interactive HTML mode
        if args.out:
            output_path = Path(args.out)
        else:
            output_path = Path(f"/tmp/knob_{args.material}_interactive.html")

        create_interactive_html(args.base_dir, args.material, output_path)

        # Try to open in browser
        import subprocess
        try:
            if sys.platform == "darwin":
                subprocess.run(["open", str(output_path)], check=False)
                print("   (Opened in browser)")
            elif sys.platform == "linux":
                subprocess.run(["xdg-open", str(output_path)], check=False)
                print("   (Opened in browser)")
        except:
            pass

    elif args.rotation is not None:
        # Single rotation
        print(f"Rotation: {args.rotation}°")
        print()

        composite = composite_knob(args.base_dir, args.material, args.rotation)

        if composite:
            if args.out:
                output_path = Path(args.out)
            else:
                output_path = Path(f"/tmp/knob_{args.material}_{int(args.rotation)}deg.png")

            composite.save(output_path)
            print(f"\n✅ Saved composite: {output_path}")

            # Try to open with system viewer
            import subprocess
            try:
                if sys.platform == "darwin":  # macOS
                    subprocess.run(["open", str(output_path)], check=False)
                elif sys.platform == "linux":
                    subprocess.run(["xdg-open", str(output_path)], check=False)
                print("   (Opened in system viewer)")
            except:
                pass

    else:
        # Rotation sequence (original behavior)
        if args.out:
            output_path = Path(args.out)
        else:
            output_path = Path(f"/tmp/knob_{args.material}_sequence.png")

        create_rotation_sequence(args.base_dir, args.material, output_path)

        # Try to open
        import subprocess
        try:
            if sys.platform == "darwin":
                subprocess.run(["open", str(output_path)], check=False)
            print("   (Opened in system viewer)")
        except:
            pass

    print("\n" + "=" * 70)
    print("✨ Preview complete!")
    print("=" * 70)


if __name__ == "__main__":
    main()