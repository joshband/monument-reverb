#!/usr/bin/env python3
"""Create visual contact sheet of all knob assets.

Generates a grid layout showing all existing assets with labels,
making it easy to review and select which ones to use.

Usage:
    python3 tools/create_asset_contact_sheet.py --input "UI Mockup/images/knobs" --output "UI Mockup/asset_contact_sheet.png"
"""

import argparse
import sys
from pathlib import Path
from typing import List, Tuple

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:
    print("❌ Missing Pillow. Install with: pip3 install Pillow")
    sys.exit(1)


def create_thumbnail(img_path: Path, size: Tuple[int, int] = (256, 256)) -> Image.Image:
    """Create thumbnail with padding and border.

    Args:
        img_path: Path to image
        size: Thumbnail size (width, height)

    Returns:
        Thumbnail image
    """
    img = Image.open(img_path)

    # Convert to RGBA
    if img.mode != 'RGBA':
        img = img.convert('RGBA')

    # Resize to fit within bounds
    img.thumbnail(size, Image.Resampling.LANCZOS)

    # Create padded background
    thumb = Image.new('RGBA', size, (50, 50, 55, 255))

    # Center image
    x = (size[0] - img.width) // 2
    y = (size[1] - img.height) // 2
    thumb.paste(img, (x, y), img)

    return thumb


def create_contact_sheet(
    image_paths: List[Path],
    output_path: Path,
    cols: int = 4,
    thumb_size: int = 256,
    label_height: int = 60,
    title: str = "Monument Knob Assets"
):
    """Create contact sheet grid.

    Args:
        image_paths: List of image file paths
        output_path: Output path for contact sheet
        cols: Number of columns
        thumb_size: Thumbnail size (square)
        label_height: Height of label area below thumbnail
        title: Sheet title
    """
    if not image_paths:
        print("❌ No images to process")
        return

    # Calculate grid dimensions
    rows = (len(image_paths) + cols - 1) // cols

    cell_width = thumb_size + 20
    cell_height = thumb_size + label_height + 20

    title_height = 80
    margin = 20

    sheet_width = cols * cell_width + margin * 2
    sheet_height = rows * cell_height + title_height + margin * 2

    # Create canvas
    canvas = Image.new('RGB', (sheet_width, sheet_height), (30, 32, 35))
    draw = ImageDraw.Draw(canvas)

    # Try to load font
    try:
        title_font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 32)
        label_font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 14)
    except:
        title_font = ImageFont.load_default()
        label_font = ImageFont.load_default()

    # Draw title
    title_bbox = draw.textbbox((0, 0), title, font=title_font)
    title_width = title_bbox[2] - title_bbox[0]
    title_x = (sheet_width - title_width) // 2
    draw.text((title_x, margin + 20), title, fill=(220, 220, 220), font=title_font)

    # Draw grid
    for i, img_path in enumerate(image_paths):
        row = i // cols
        col = i % cols

        x = margin + col * cell_width + 10
        y = title_height + margin + row * cell_height + 10

        # Create thumbnail
        try:
            thumb = create_thumbnail(img_path, (thumb_size, thumb_size))
            canvas.paste(thumb, (x, y), thumb)
        except Exception as e:
            print(f"⚠️  Failed to process {img_path.name}: {e}")
            continue

        # Draw label
        label_y = y + thumb_size + 10

        # Shorten filename if too long
        label = img_path.stem
        if len(label) > 30:
            label = label[:27] + "..."

        # Draw label background
        label_bbox = draw.textbbox((x, label_y), label, font=label_font)
        draw.rectangle(
            [x - 2, label_y - 2, x + thumb_size + 2, label_bbox[3] + 2],
            fill=(40, 42, 45)
        )

        # Draw label text
        draw.text((x + 5, label_y), label, fill=(180, 180, 185), font=label_font)

        # Draw index number
        index_text = f"#{i+1}"
        draw.text((x + 5, y + 5), index_text, fill=(100, 180, 255), font=label_font)

    # Save
    canvas.save(output_path, "PNG")
    print(f"✅ Contact sheet saved: {output_path}")


def main():
    parser = argparse.ArgumentParser(description="Create asset contact sheet")

    parser.add_argument(
        "--input",
        type=str,
        default="UI Mockup/images/knobs",
        help="Input directory"
    )

    parser.add_argument(
        "--output",
        type=str,
        default="UI Mockup/knob_assets_contact_sheet.png",
        help="Output file path"
    )

    parser.add_argument(
        "--cols",
        type=int,
        default=4,
        help="Number of columns"
    )

    parser.add_argument(
        "--thumb-size",
        type=int,
        default=256,
        help="Thumbnail size in pixels"
    )

    args = parser.parse_args()

    input_dir = Path(args.input)
    output_path = Path(args.output)

    if not input_dir.exists():
        print(f"❌ Input directory not found: {input_dir}")
        sys.exit(1)

    # Get all PNG files
    image_paths = sorted(input_dir.glob("*.png"))

    # Categorize
    stone_knobs = [f for f in image_paths if "stone_rotary_knob" in f.name]
    led_glows = [f for f in image_paths if "amorphous" in f.name or "glowing_led" in f.name]
    switches = [f for f in image_paths if "switch" in f.name]
    pbr_maps = [f for f in image_paths if f.name in ["Albedo.png", "AORoughness.png", "EnvironmentalGlow.png", "RGB Image.png", "RGBA-Image.png"]]

    print(f"\n📊 Creating contact sheets...\n")

    # Create separate sheets for each category
    if stone_knobs:
        print(f"Stone knobs: {len(stone_knobs)} assets")
        create_contact_sheet(
            stone_knobs,
            output_path.parent / "contact_sheet_stone_knobs.png",
            cols=args.cols,
            thumb_size=args.thumb_size,
            title="Stone Knob Variants"
        )

    if led_glows:
        print(f"LED/Crystal glows: {len(led_glows)} assets")
        create_contact_sheet(
            led_glows,
            output_path.parent / "contact_sheet_crystal_glows.png",
            cols=args.cols,
            thumb_size=args.thumb_size,
            title="Crystal Glow Overlays"
        )

    if switches:
        print(f"Switches: {len(switches)} assets")
        create_contact_sheet(
            switches,
            output_path.parent / "contact_sheet_switches.png",
            cols=args.cols,
            thumb_size=args.thumb_size,
            title="Stone Switches"
        )

    # Create combined overview
    print(f"\nCreating combined overview...")
    create_contact_sheet(
        image_paths,
        output_path,
        cols=6,
        thumb_size=200,
        title="All Monument Knob Assets"
    )

    print(f"\n✅ All contact sheets created in: {output_path.parent}")


if __name__ == "__main__":
    main()
