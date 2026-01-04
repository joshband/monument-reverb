#!/usr/bin/env python3
"""
Analyze knob images for isolation/masking suitability
Identifies images with clean backgrounds, good contrast, and proper composition
"""
import os
from PIL import Image
import numpy as np
from pathlib import Path

def analyze_image(image_path):
    """Analyze a single knob image for isolation suitability"""
    img = Image.open(image_path)
    img_array = np.array(img)

    # Get image properties
    width, height = img.size
    has_alpha = img.mode == 'RGBA'

    # Analyze corners (should be white/transparent for clean isolation)
    corner_size = 50
    corners = [
        img_array[:corner_size, :corner_size],  # Top-left
        img_array[:corner_size, -corner_size:],  # Top-right
        img_array[-corner_size:, :corner_size],  # Bottom-left
        img_array[-corner_size:, -corner_size:]  # Bottom-right
    ]

    # Check if corners are mostly white (good for masking)
    white_corners = 0
    for corner in corners:
        if has_alpha:
            avg_brightness = np.mean(corner[:, :, :3])  # RGB only
            avg_alpha = np.mean(corner[:, :, 3])
            if avg_brightness > 240 or avg_alpha < 15:  # Nearly white or transparent
                white_corners += 1
        else:
            avg_brightness = np.mean(corner)
            if avg_brightness > 240:  # Nearly white
                white_corners += 1

    # Calculate contrast (good for edge detection)
    if has_alpha:
        gray = np.mean(img_array[:, :, :3], axis=2)
    else:
        gray = np.mean(img_array, axis=2) if len(img_array.shape) == 3 else img_array

    contrast = np.std(gray)

    # Check aspect ratio (should be roughly square)
    aspect_ratio = width / height
    is_square = 0.9 <= aspect_ratio <= 1.1

    # Calculate score
    score = 0
    score += (white_corners / 4) * 40  # 40 points for clean background
    score += min(contrast / 80 * 40, 40)  # 40 points for good contrast
    score += 20 if is_square else 0  # 20 points for square aspect

    return {
        'path': image_path,
        'filename': os.path.basename(image_path),
        'size': (width, height),
        'has_alpha': has_alpha,
        'white_corners': white_corners,
        'contrast': round(contrast, 2),
        'aspect_ratio': round(aspect_ratio, 2),
        'is_square': is_square,
        'score': round(score, 2)
    }

def main():
    knobs_dir = Path.home() / 'Desktop' / 'knobs'

    if not knobs_dir.exists():
        print(f"Directory not found: {knobs_dir}")
        return

    print("Analyzing knob images...")
    print("=" * 80)

    # Analyze all PNG images
    results = []
    for img_path in knobs_dir.glob('*.png'):
        try:
            result = analyze_image(img_path)
            results.append(result)
        except Exception as e:
            print(f"Error analyzing {img_path.name}: {e}")

    # Sort by score (best first)
    results.sort(key=lambda x: x['score'], reverse=True)

    # Print top candidates
    print("\n🏆 TOP 20 CANDIDATES FOR ISOLATION/MASKING")
    print("=" * 80)
    print(f"{'Rank':<5} {'Score':<8} {'Size':<12} {'Contrast':<10} {'Alpha':<7} {'Filename':<40}")
    print("-" * 80)

    for i, r in enumerate(results[:20], 1):
        size_str = f"{r['size'][0]}x{r['size'][1]}"
        alpha_str = "Yes" if r['has_alpha'] else "No"
        filename = r['filename'][:40] + "..." if len(r['filename']) > 40 else r['filename']
        print(f"{i:<5} {r['score']:<8} {size_str:<12} {r['contrast']:<10} {alpha_str:<7} {filename}")

    # Category breakdown
    print("\n\n📊 CATEGORY BREAKDOWN (Top 20)")
    print("=" * 80)
    categories = {}
    for r in results[:20]:
        category = '_'.join(r['filename'].split('_')[:4])
        categories[category] = categories.get(category, 0) + 1

    for category, count in sorted(categories.items(), key=lambda x: x[1], reverse=True):
        print(f"{count:>3}x  {category}")

    # Export top candidates to a text file
    export_path = knobs_dir / '_top_candidates.txt'
    with open(export_path, 'w') as f:
        f.write("Top 20 Knob Images for Isolation/Masking\n")
        f.write("=" * 80 + "\n\n")
        for i, r in enumerate(results[:20], 1):
            f.write(f"{i}. {r['filename']} (score: {r['score']})\n")

    print(f"\n✅ Results exported to: {export_path}")
    print(f"\n💡 TIP: Images with score >80 are excellent candidates")
    print(f"   Images with score >60 are good candidates")

if __name__ == '__main__':
    main()