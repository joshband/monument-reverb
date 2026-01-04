#!/usr/bin/env python3
"""Generate Midjourney prompts for Monument Reverb knobs.

Creates prompts for industrial/brutalist knobs with multiple layers:
- Base body (rotating)
- Detail ring (static)
- Indicator (rotating)
- Center cap (static)
"""

from dataclasses import dataclass
from pathlib import Path
import json


@dataclass
class KnobLayer:
    """Specification for a single knob layer."""
    name: str
    description: str
    rotates: bool
    base_prompt: str
    variations: int = 4  # Generate 4 variations for Materialize median fusion


def generate_base_body_prompts() -> list[dict]:
    """Generate prompts for knob base body layer."""
    base_template = (
        "top-down view, isolated circular knob, {material}, {pattern}, "
        "512px diameter, centered on white background, photorealistic, "
        "studio lighting, soft shadows, ambient occlusion, {finish}, "
        "deadpan composition, knolling arrangement --ar 1:1 --v 6.1 "
        "--style raw --quality 2"
    )

    materials = [
        ("concrete", "brutalist architectural texture, weathered", "matte finish"),
        ("stone", "carved granite with radial segments", "natural texture"),
        ("metal", "brushed aluminum with concentric rings", "semi-gloss finish"),
        ("basalt", "dark volcanic rock with geometric patterns", "rough texture"),
    ]

    prompts = []
    for material, pattern, finish in materials:
        for var in range(4):
            seed = 10000 + (len(prompts) * 100) + var
            prompt = base_template.format(
                material=material,
                pattern=pattern,
                finish=finish
            )
            prompts.append({
                "layer": "base_body",
                "material": material,
                "variation": var + 1,
                "seed": seed,
                "prompt": f"/imagine {prompt} --seed {seed}",
                "output_name": f"knob_base_{material}_v{var+1}.png"
            })

    return prompts


def generate_indicator_prompts() -> list[dict]:
    """Generate prompts for rotating indicator layer."""
    indicator_template = (
        "top-down view, isolated knob indicator pointer, single straight line "
        "from center to edge, {material} on transparent background, "
        "512px canvas, minimalist, clean geometry, studio lighting, "
        "high contrast with background, deadpan --ar 1:1 --v 6.1 "
        "--style raw --quality 2"
    )

    indicators = [
        ("metal", "brushed metal bar"),
        ("carved", "carved notch or groove"),
        ("inset", "recessed line"),
        ("raised", "embossed ridge"),
    ]

    prompts = []
    for style, material_desc in indicators:
        for var in range(4):
            seed = 20000 + (len(prompts) * 100) + var
            prompt = indicator_template.format(material=material_desc)
            prompts.append({
                "layer": "indicator",
                "style": style,
                "variation": var + 1,
                "seed": seed,
                "prompt": f"/imagine {prompt} --seed {seed}",
                "output_name": f"knob_indicator_{style}_v{var+1}.png"
            })

    return prompts


def generate_detail_ring_prompts() -> list[dict]:
    """Generate prompts for static detail ring (scale markings)."""
    ring_template = (
        "top-down view, isolated circular ring with scale markings, "
        "{marking_style}, 512px diameter, centered, transparent center, "
        "white background, subtle engraving, industrial design, "
        "12 tick marks at clock positions, minimalist --ar 1:1 --v 6.1 "
        "--style raw --quality 2"
    )

    marking_styles = [
        "engraved tick marks",
        "raised geometric segments",
        "carved radial lines",
        "dot indicators",
    ]

    prompts = []
    for style in marking_styles:
        for var in range(4):
            seed = 30000 + (len(prompts) * 100) + var
            prompt = ring_template.format(marking_style=style)
            prompts.append({
                "layer": "detail_ring",
                "style": style,
                "variation": var + 1,
                "seed": seed,
                "prompt": f"/imagine {prompt} --seed {seed}",
                "output_name": f"knob_ring_{style.replace(' ', '_')}_v{var+1}.png"
            })

    return prompts


def generate_center_cap_prompts() -> list[dict]:
    """Generate prompts for static center cap."""
    cap_template = (
        "top-down view, isolated circular center cap, {material}, "
        "80px diameter disc on white background, centered in 512px canvas, "
        "photorealistic, studio lighting, {detail}, minimalist --ar 1:1 "
        "--v 6.1 --style raw --quality 2"
    )

    caps = [
        ("brushed metal", "subtle concentric circles"),
        ("polished stone", "smooth finish with veining"),
        ("matte concrete", "subtle texture grain"),
        ("dark metal", "embossed logo or pattern"),
    ]

    prompts = []
    for material, detail in caps:
        for var in range(4):
            seed = 40000 + (len(prompts) * 100) + var
            prompt = cap_template.format(material=material, detail=detail)
            prompts.append({
                "layer": "center_cap",
                "material": material,
                "variation": var + 1,
                "seed": seed,
                "prompt": f"/imagine {prompt} --seed {seed}",
                "output_name": f"knob_cap_{material.replace(' ', '_')}_v{var+1}.png"
            })

    return prompts


def generate_all_prompts() -> dict:
    """Generate all knob layer prompts organized by layer."""
    return {
        "base_body": generate_base_body_prompts(),
        "indicator": generate_indicator_prompts(),
        "detail_ring": generate_detail_ring_prompts(),
        "center_cap": generate_center_cap_prompts(),
    }


def save_prompts(output_dir: Path) -> None:
    """Save generated prompts to organized directory structure."""
    output_dir.mkdir(parents=True, exist_ok=True)

    all_prompts = generate_all_prompts()

    # Save master manifest
    manifest_path = output_dir / "knob_prompts_manifest.json"
    with manifest_path.open("w") as f:
        json.dump(all_prompts, f, indent=2)

    print(f"✅ Saved manifest: {manifest_path}")
    print(f"\nGenerated {sum(len(prompts) for prompts in all_prompts.values())} total prompts")
    print("\n=== Layer Summary ===")
    for layer_name, prompts in all_prompts.items():
        print(f"{layer_name}: {len(prompts)} prompts")

    # Save separate text files for easy copy-paste to Midjourney
    for layer_name, prompts in all_prompts.items():
        layer_file = output_dir / f"{layer_name}_prompts.txt"
        with layer_file.open("w") as f:
            f.write(f"# {layer_name.replace('_', ' ').title()} Prompts\n\n")
            for prompt_data in prompts:
                f.write(f"## {prompt_data['output_name']}\n")
                f.write(f"{prompt_data['prompt']}\n\n")
        print(f"  Saved: {layer_file}")

    # Generate batch organization guide
    guide_path = output_dir / "ORGANIZATION_GUIDE.md"
    with guide_path.open("w") as f:
        f.write("# Knob Asset Organization Guide\n\n")
        f.write("## Step 1: Generate Renders in Midjourney\n\n")
        f.write("Copy prompts from the `*_prompts.txt` files and run in Midjourney.\n\n")
        f.write("## Step 2: Organize into Job Folders\n\n")
        f.write("Create this directory structure for Materialize processing:\n\n")
        f.write("```\n")
        f.write("mj_knob_jobs/\n")

        for layer_name, prompts in all_prompts.items():
            # Group by material/style
            groups = {}
            for p in prompts:
                key = p.get('material') or p.get('style')
                if key not in groups:
                    groups[key] = []
                groups[key].append(p)

            for group_name, group_prompts in groups.items():
                job_id = f"{layer_name}_{group_name.replace(' ', '_')}"
                f.write(f"├── {job_id}/\n")
                for p in group_prompts:
                    f.write(f"│   ├── {p['output_name']}\n")

        f.write("```\n\n")
        f.write("## Step 3: Run Materialize Pipeline\n\n")
        f.write("```bash\n")
        f.write("materialize \\\n")
        f.write("  --in mj_knob_jobs \\\n")
        f.write("  --out monument-reverb/assets/knobs_pbr \\\n")
        f.write("  --size 512 \\\n")
        f.write("  --workers 4 \\\n")
        f.write("  --overwrite\n")
        f.write("```\n\n")
        f.write("## Step 4: Extract Alpha Channels\n\n")
        f.write("```bash\n")
        f.write("python scripts/extract_knob_layers.py \\\n")
        f.write("  --in assets/knobs_pbr \\\n")
        f.write("  --out assets/ui/knobs\n")
        f.write("```\n")

    print(f"\n✅ Saved organization guide: {guide_path}")


if __name__ == "__main__":
    import sys

    if len(sys.argv) > 1:
        output_dir = Path(sys.argv[1])
    else:
        output_dir = Path("~/Desktop/monument-knob-prompts").expanduser()

    print(f"🎨 Generating Monument Knob Prompts")
    print(f"📁 Output directory: {output_dir}\n")

    save_prompts(output_dir)

    print("\n" + "="*60)
    print("✨ Next steps:")
    print("1. Open Midjourney and paste prompts from the generated .txt files")
    print("2. Download renders and organize into job folders (see ORGANIZATION_GUIDE.md)")
    print("3. Run Materialize pipeline to extract PBR maps")
    print("4. Run extract_knob_layers.py to create final alpha-channel layers")
    print("="*60)