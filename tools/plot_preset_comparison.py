#!/usr/bin/env python3
"""
Monument Reverb - Preset Comparison Visualization Tool

Generates comprehensive comparison charts for all analyzed presets:
- RT60 comparison bar chart
- Frequency response comparison heatmap
- Statistical summaries

Usage:
    python3 plot_preset_comparison.py <baseline_dir> [--output output_dir]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Optional

try:
    import numpy as np
    import matplotlib.pyplot as plt
    import matplotlib.cm as cm
    from matplotlib.patches import Rectangle
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("\nInstall dependencies:")
    print("  pip3 install numpy matplotlib")
    sys.exit(1)


def load_preset_metrics(baseline_dir: Path) -> Dict:
    """Load all preset metrics from JSON files."""
    presets = {}

    for i in range(37):
        preset_dir = baseline_dir / f"preset_{i:02d}"
        if not preset_dir.exists():
            continue

        preset_data = {
            'index': i,
            'name': f"Preset {i:02d}",
            'rt60': None,
            'freq_response': None
        }

        # Load RT60 metrics
        rt60_file = preset_dir / "rt60_metrics.json"
        if rt60_file.exists():
            try:
                with open(rt60_file) as f:
                    preset_data['rt60'] = json.load(f)
            except Exception as e:
                print(f"⚠ Failed to load RT60 for preset {i}: {e}")

        # Load frequency response metrics
        freq_file = preset_dir / "freq_metrics.json"
        if freq_file.exists():
            try:
                with open(freq_file) as f:
                    preset_data['freq_response'] = json.load(f)
            except Exception as e:
                print(f"⚠ Failed to load frequency response for preset {i}: {e}")

        presets[i] = preset_data

    return presets


def plot_rt60_comparison(presets: Dict, output_path: Path):
    """Generate RT60 comparison bar chart."""
    indices = []
    rt60_values = []
    colors = []

    for i in sorted(presets.keys()):
        if presets[i]['rt60'] is None:
            continue

        rt60_data = presets[i]['rt60']
        # RT60 can be at top level or under 'broadband' key
        rt60_value = rt60_data.get('rt60_seconds') or rt60_data.get('broadband', {}).get('rt60_seconds')

        if rt60_value is not None:
            indices.append(i)
            rt60_values.append(rt60_value)

            # Color code by RT60 length
            if rt60_value < 1.0:
                colors.append('#2ecc71')  # Green - short
            elif rt60_value < 3.0:
                colors.append('#3498db')  # Blue - medium
            elif rt60_value < 6.0:
                colors.append('#f39c12')  # Orange - long
            else:
                colors.append('#e74c3c')  # Red - very long

    if not indices:
        print("⚠ No RT60 data available for plotting")
        return

    # Create figure
    fig, ax = plt.subplots(figsize=(16, 6))

    bars = ax.bar(indices, rt60_values, color=colors, alpha=0.7, edgecolor='black', linewidth=0.5)

    # Add value labels on bars
    for idx, (i, val) in enumerate(zip(indices, rt60_values)):
        ax.text(i, val + 0.1, f'{val:.2f}s', ha='center', va='bottom', fontsize=7)

    # Formatting
    ax.set_xlabel('Preset Index', fontsize=12, fontweight='bold')
    ax.set_ylabel('RT60 (seconds)', fontsize=12, fontweight='bold')
    ax.set_title('Monument Reverb - RT60 Comparison Across All Presets',
                 fontsize=14, fontweight='bold', pad=20)
    ax.grid(True, axis='y', alpha=0.3, linestyle='--')
    ax.set_axisbelow(True)

    # Add reference lines
    ax.axhline(1.0, color='gray', linestyle=':', linewidth=1, alpha=0.5, label='1s (Short)')
    ax.axhline(3.0, color='gray', linestyle=':', linewidth=1, alpha=0.5, label='3s (Medium)')
    ax.axhline(6.0, color='gray', linestyle=':', linewidth=1, alpha=0.5, label='6s (Long)')

    # Set x-axis to show all preset indices
    ax.set_xticks(range(0, 37, 2))
    ax.set_xlim(-1, 37)

    # Legend
    ax.legend(loc='upper right', fontsize=9)

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()

    print(f"✓ Saved RT60 comparison chart: {output_path}")


def plot_frequency_response_heatmap(presets: Dict, output_path: Path):
    """Generate frequency response comparison heatmap."""
    # Extract frequency response data for all presets
    preset_indices = []
    flatness_values = []
    band_data = {
        'Sub': [], 'Bass': [], 'Low-Mid': [], 'Mid': [],
        'High-Mid': [], 'Presence': [], 'Brilliance': []
    }

    for i in sorted(presets.keys()):
        if presets[i]['freq_response'] is None:
            continue

        freq_data = presets[i]['freq_response']
        overall = freq_data.get('overall', {})
        bands = freq_data.get('octave_bands', {})

        flatness = overall.get('flatness_std_db')
        if flatness is not None:
            preset_indices.append(i)
            flatness_values.append(flatness)

            # Extract band averages
            for band_name in band_data.keys():
                band_info = bands.get(band_name, {})
                avg_db = band_info.get('average_db', 0.0)
                band_data[band_name].append(avg_db)

    if not preset_indices:
        print("⚠ No frequency response data available for plotting")
        return

    # Create heatmap matrix
    band_names = list(band_data.keys())
    matrix = np.array([band_data[name] for name in band_names])

    # Create figure
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(16, 8),
                                    gridspec_kw={'height_ratios': [3, 1]})

    # Heatmap
    im = ax1.imshow(matrix, aspect='auto', cmap='RdYlGn_r',
                    vmin=-15, vmax=5, interpolation='nearest')

    # Set ticks
    ax1.set_yticks(range(len(band_names)))
    ax1.set_yticklabels(band_names, fontsize=10)
    ax1.set_xticks(range(len(preset_indices)))
    ax1.set_xticklabels(preset_indices, fontsize=8)

    # Labels
    ax1.set_xlabel('Preset Index', fontsize=12, fontweight='bold')
    ax1.set_ylabel('Frequency Band', fontsize=12, fontweight='bold')
    ax1.set_title('Frequency Response by Band (dB relative to 0dB reference)',
                  fontsize=13, fontweight='bold', pad=15)

    # Colorbar
    cbar = plt.colorbar(im, ax=ax1, orientation='vertical', pad=0.02)
    cbar.set_label('Magnitude (dB)', fontsize=10, fontweight='bold')

    # Add grid
    ax1.set_xticks(np.arange(-0.5, len(preset_indices), 1), minor=True)
    ax1.set_yticks(np.arange(-0.5, len(band_names), 1), minor=True)
    ax1.grid(which='minor', color='gray', linestyle='-', linewidth=0.5, alpha=0.3)

    # Flatness comparison bar chart
    ax2.bar(preset_indices, flatness_values, color='steelblue', alpha=0.7,
            edgecolor='black', linewidth=0.5)
    ax2.set_xlabel('Preset Index', fontsize=12, fontweight='bold')
    ax2.set_ylabel('Flatness (±dB)', fontsize=10, fontweight='bold')
    ax2.set_title('Overall Frequency Response Flatness', fontsize=11, fontweight='bold')
    ax2.grid(True, axis='y', alpha=0.3, linestyle='--')
    ax2.axhline(3, color='green', linestyle=':', linewidth=1, alpha=0.7, label='Excellent (±3dB)')
    ax2.axhline(6, color='orange', linestyle=':', linewidth=1, alpha=0.7, label='Good (±6dB)')
    ax2.axhline(10, color='red', linestyle=':', linewidth=1, alpha=0.7, label='Fair (±10dB)')
    ax2.legend(loc='upper right', fontsize=8)
    ax2.set_xlim(-1, 37)
    ax2.set_xticks(range(0, 37, 2))

    plt.tight_layout()
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    plt.close()

    print(f"✓ Saved frequency response heatmap: {output_path}")


def generate_summary_stats(presets: Dict, output_path: Path):
    """Generate summary statistics text file."""
    rt60_values = []
    flatness_values = []

    for i in sorted(presets.keys()):
        if presets[i]['rt60']:
            # RT60 can be at top level or under 'broadband' key
            rt60 = presets[i]['rt60'].get('rt60_seconds') or presets[i]['rt60'].get('broadband', {}).get('rt60_seconds')
            if rt60:
                rt60_values.append(rt60)

        if presets[i]['freq_response']:
            flatness = presets[i]['freq_response'].get('overall', {}).get('flatness_std_db')
            if flatness:
                flatness_values.append(flatness)

    with open(output_path, 'w') as f:
        f.write("=" * 60 + "\n")
        f.write("Monument Reverb - Preset Statistics Summary\n")
        f.write("=" * 60 + "\n\n")

        f.write(f"Total Presets Analyzed: {len(presets)}\n\n")

        if rt60_values:
            f.write("RT60 Statistics:\n")
            f.write(f"  Mean:     {np.mean(rt60_values):.3f} seconds\n")
            f.write(f"  Median:   {np.median(rt60_values):.3f} seconds\n")
            f.write(f"  Std Dev:  {np.std(rt60_values):.3f} seconds\n")
            f.write(f"  Min:      {np.min(rt60_values):.3f} seconds (Preset {rt60_values.index(np.min(rt60_values))})\n")
            f.write(f"  Max:      {np.max(rt60_values):.3f} seconds (Preset {rt60_values.index(np.max(rt60_values))})\n")
            f.write("\n")

        if flatness_values:
            f.write("Frequency Response Flatness Statistics:\n")
            f.write(f"  Mean:     ±{np.mean(flatness_values):.2f} dB\n")
            f.write(f"  Median:   ±{np.median(flatness_values):.2f} dB\n")
            f.write(f"  Std Dev:  {np.std(flatness_values):.2f} dB\n")
            f.write(f"  Min:      ±{np.min(flatness_values):.2f} dB (Preset {flatness_values.index(np.min(flatness_values))}) - Most Flat\n")
            f.write(f"  Max:      ±{np.max(flatness_values):.2f} dB (Preset {flatness_values.index(np.max(flatness_values))}) - Most Colored\n")
            f.write("\n")

            # Quality ratings
            excellent = sum(1 for f in flatness_values if f < 3)
            good = sum(1 for f in flatness_values if 3 <= f < 6)
            fair = sum(1 for f in flatness_values if 6 <= f < 10)
            colored = sum(1 for f in flatness_values if f >= 10)

            f.write("Quality Distribution:\n")
            f.write(f"  Excellent (±3dB):  {excellent} presets ({100*excellent/len(flatness_values):.1f}%)\n")
            f.write(f"  Good (±6dB):       {good} presets ({100*good/len(flatness_values):.1f}%)\n")
            f.write(f"  Fair (±10dB):      {fair} presets ({100*fair/len(flatness_values):.1f}%)\n")
            f.write(f"  Colored (>±10dB):  {colored} presets ({100*colored/len(flatness_values):.1f}%)\n")

        f.write("\n" + "=" * 60 + "\n")

    print(f"✓ Saved summary statistics: {output_path}")


def main():
    parser = argparse.ArgumentParser(
        description='Generate preset comparison visualizations for Monument Reverb',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('baseline_dir', type=Path,
                       help='Directory containing analyzed preset data')
    parser.add_argument('--output', '-o', type=Path,
                       default=Path('./test-results/comparisons'),
                       help='Output directory for charts (default: ./test-results/comparisons)')

    args = parser.parse_args()

    if not args.baseline_dir.exists():
        print(f"Error: Baseline directory not found: {args.baseline_dir}")
        return 1

    # Create output directory
    args.output.mkdir(parents=True, exist_ok=True)

    print("\n" + "=" * 60)
    print("Monument Reverb - Preset Comparison Tool")
    print("=" * 60 + "\n")

    # Load all preset metrics
    print(f"▸ Loading preset metrics from: {args.baseline_dir}")
    presets = load_preset_metrics(args.baseline_dir)
    print(f"  Loaded {len(presets)} presets\n")

    # Generate visualizations
    print("▸ Generating comparison charts...")

    plot_rt60_comparison(presets, args.output / "rt60_comparison.png")
    plot_frequency_response_heatmap(presets, args.output / "frequency_response_comparison.png")
    generate_summary_stats(presets, args.output / "summary_statistics.txt")

    print("\n" + "=" * 60)
    print("✓ Comparison visualizations complete!")
    print("=" * 60)
    print(f"\nOutput saved to: {args.output}")
    print("")

    return 0


if __name__ == '__main__':
    sys.exit(main())
