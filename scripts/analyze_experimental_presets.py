#!/usr/bin/env python3
"""
Monument Reverb - Experimental Presets Analysis
Analyzes and visualizes RT60 and frequency response data for all 8 presets
"""

import json
import os
from pathlib import Path
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import numpy as np

# Configuration
PRESET_BASE = Path("test-results/preset-baseline")
OUTPUT_DIR = Path("test-results/experimental-analysis")
OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

# Preset definitions
PRESETS = {
    0: {"name": "Monolith", "type": "original", "expected_rt60": (8, 15)},
    1: {"name": "Cathedral", "type": "original", "expected_rt60": (6, 12)},
    2: {"name": "Infinite Hall", "type": "original", "expected_rt60": (15, 25)},
    3: {"name": "Infinite Abyss", "type": "experimental", "expected_rt60": (10, 30)},
    4: {"name": "Quantum Tunneling", "type": "experimental", "expected_rt60": (15, 20)},
    5: {"name": "Time Dissolution", "type": "experimental", "expected_rt60": (6, 30)},
    6: {"name": "Crystalline Void", "type": "experimental", "expected_rt60": (15, 25)},
    7: {"name": "Hyperdimensional Fold", "type": "experimental", "expected_rt60": (10, 30)},
}


def load_rt60_data(preset_num):
    """Load RT60 metrics for a preset"""
    preset_dir = PRESET_BASE / f"preset_{preset_num:02d}"
    rt60_file = preset_dir / "rt60_metrics.json"

    if not rt60_file.exists():
        return None

    with open(rt60_file) as f:
        data = json.load(f)

    return {
        "rt60": data["broadband"]["rt60_seconds"],
        "peak": data["_metadata"]["peak"],
        "rms": data["_metadata"]["rms"],
        "dynamic_range_db": data["_metadata"]["dynamic_range_db"],
    }


def load_all_rt60_data():
    """Load RT60 data for all presets"""
    data = {}
    for preset_num, info in PRESETS.items():
        rt60_data = load_rt60_data(preset_num)
        if rt60_data:
            data[preset_num] = {**info, **rt60_data}
    return data


def create_rt60_comparison_plot(data):
    """Create RT60 comparison plot (original vs experimental)"""
    fig, ax = plt.subplots(figsize=(14, 8))

    # Separate original and experimental presets
    original_presets = {k: v for k, v in data.items() if v["type"] == "original"}
    experimental_presets = {k: v for k, v in data.items() if v["type"] == "experimental"}

    # Plot settings
    bar_width = 0.6
    colors = {"original": "#3498db", "experimental": "#e74c3c"}

    # Plot original presets
    for i, (num, preset) in enumerate(original_presets.items()):
        ax.bar(i, preset["rt60"], bar_width, color=colors["original"],
               alpha=0.8, edgecolor='black', linewidth=1.5)

        # Add expected range as error bar
        expected_min, expected_max = preset["expected_rt60"]
        expected_mid = (expected_min + expected_max) / 2
        expected_range = expected_max - expected_min
        ax.errorbar(i, expected_mid, yerr=expected_range/2, fmt='none',
                   color='gray', capsize=8, capthick=2, alpha=0.6, zorder=1)

    # Plot experimental presets
    offset = len(original_presets) + 0.5
    for i, (num, preset) in enumerate(experimental_presets.items()):
        ax.bar(offset + i, preset["rt60"], bar_width, color=colors["experimental"],
               alpha=0.8, edgecolor='black', linewidth=1.5)

        # Add expected range
        expected_min, expected_max = preset["expected_rt60"]
        expected_mid = (expected_min + expected_max) / 2
        expected_range = expected_max - expected_min
        ax.errorbar(offset + i, expected_mid, yerr=expected_range/2, fmt='none',
                   color='gray', capsize=8, capthick=2, alpha=0.6, zorder=1)

    # Labels and formatting
    all_names = [data[k]["name"] for k in sorted(data.keys())]
    ax.set_xticks(range(len(all_names)))
    ax.set_xticklabels(all_names, rotation=45, ha='right', fontsize=11)

    ax.set_ylabel('RT60 (seconds)', fontsize=13, fontweight='bold')
    ax.set_title('Monument Reverb - RT60 Comparison\nOriginal vs Experimental Presets',
                fontsize=15, fontweight='bold', pad=20)

    # Add legend
    original_patch = mpatches.Patch(color=colors["original"], label='Original Presets')
    experimental_patch = mpatches.Patch(color=colors["experimental"], label='Experimental Presets')
    expected_line = mpatches.Patch(color='gray', alpha=0.6, label='Expected Range')
    ax.legend(handles=[original_patch, experimental_patch, expected_line],
             loc='upper left', fontsize=11)

    # Grid
    ax.grid(axis='y', alpha=0.3, linestyle='--')
    ax.set_axisbelow(True)

    # Add 30-second capture limit line
    ax.axhline(y=30, color='red', linestyle='--', linewidth=2, alpha=0.5,
              label='30s Capture Limit')

    plt.tight_layout()
    output_path = OUTPUT_DIR / "rt60_comparison.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"✓ Saved RT60 comparison plot: {output_path}")
    plt.close()


def create_rt60_delta_plot(data):
    """Create plot showing difference from expected RT60 midpoint"""
    fig, ax = plt.subplots(figsize=(14, 6))

    experimental_presets = {k: v for k, v in data.items() if v["type"] == "experimental"}

    names = []
    deltas = []
    colors_list = []

    for num, preset in experimental_presets.items():
        expected_min, expected_max = preset["expected_rt60"]
        expected_mid = (expected_min + expected_max) / 2
        delta = preset["rt60"] - expected_mid

        names.append(preset["name"])
        deltas.append(delta)
        colors_list.append('#2ecc71' if abs(delta) < 5 else '#e67e22')

    bars = ax.barh(names, deltas, color=colors_list, alpha=0.8, edgecolor='black', linewidth=1.5)

    ax.axvline(x=0, color='black', linestyle='-', linewidth=2)
    ax.set_xlabel('RT60 Delta from Expected Midpoint (seconds)', fontsize=12, fontweight='bold')
    ax.set_title('Experimental Presets - RT60 Deviation from Expected', fontsize=14, fontweight='bold')
    ax.grid(axis='x', alpha=0.3, linestyle='--')

    # Add value labels
    for i, (bar, delta) in enumerate(zip(bars, deltas)):
        ax.text(delta + (0.5 if delta > 0 else -0.5), i, f'{delta:+.1f}s',
               va='center', ha='left' if delta > 0 else 'right', fontsize=10, fontweight='bold')

    plt.tight_layout()
    output_path = OUTPUT_DIR / "rt60_delta.png"
    plt.savefig(output_path, dpi=150, bbox_inches='tight')
    print(f"✓ Saved RT60 delta plot: {output_path}")
    plt.close()


def create_statistics_report(data):
    """Generate comprehensive statistics report"""
    report_lines = []
    report_lines.append("=" * 80)
    report_lines.append("MONUMENT REVERB - EXPERIMENTAL PRESETS ANALYSIS REPORT")
    report_lines.append("=" * 80)
    report_lines.append("")

    # Overall statistics
    report_lines.append("## OVERALL STATISTICS")
    report_lines.append("-" * 80)

    original_rt60s = [v["rt60"] for v in data.values() if v["type"] == "original"]
    experimental_rt60s = [v["rt60"] for v in data.values() if v["type"] == "experimental"]

    report_lines.append(f"Original Presets (n={len(original_rt60s)}):")
    report_lines.append(f"  Mean RT60:   {np.mean(original_rt60s):.2f}s")
    report_lines.append(f"  Median RT60: {np.median(original_rt60s):.2f}s")
    report_lines.append(f"  Range:       {np.min(original_rt60s):.2f}s - {np.max(original_rt60s):.2f}s")
    report_lines.append("")

    report_lines.append(f"Experimental Presets (n={len(experimental_rt60s)}):")
    report_lines.append(f"  Mean RT60:   {np.mean(experimental_rt60s):.2f}s")
    report_lines.append(f"  Median RT60: {np.median(experimental_rt60s):.2f}s")
    report_lines.append(f"  Range:       {np.min(experimental_rt60s):.2f}s - {np.max(experimental_rt60s):.2f}s")
    report_lines.append("")

    rt60_increase_pct = (np.mean(experimental_rt60s) / np.mean(original_rt60s) - 1) * 100
    report_lines.append(f"Average RT60 Increase: {rt60_increase_pct:+.1f}%")
    report_lines.append("")

    # Individual preset analysis
    report_lines.append("## PRESET-BY-PRESET ANALYSIS")
    report_lines.append("-" * 80)
    report_lines.append("")

    for num in sorted(data.keys()):
        preset = data[num]
        report_lines.append(f"### Preset {num}: {preset['name']} ({preset['type'].upper()})")
        report_lines.append(f"  Measured RT60:    {preset['rt60']:.2f}s")

        expected_min, expected_max = preset["expected_rt60"]
        expected_mid = (expected_min + expected_max) / 2
        delta = preset["rt60"] - expected_mid

        report_lines.append(f"  Expected Range:   {expected_min:.0f}s - {expected_max:.0f}s")
        report_lines.append(f"  Delta from Mid:   {delta:+.2f}s")

        # Validation
        in_range = expected_min <= preset["rt60"] <= expected_max
        status = "✓ PASS" if in_range else "⚠ OUT OF RANGE"
        report_lines.append(f"  Validation:       {status}")

        # Additional metrics
        report_lines.append(f"  Peak:             {preset['peak']:.0f}")
        report_lines.append(f"  RMS:              {preset['rms']:.2f}")
        report_lines.append(f"  Dynamic Range:    {preset['dynamic_range_db']:.1f} dB")
        report_lines.append("")

    # Key findings
    report_lines.append("## KEY FINDINGS")
    report_lines.append("-" * 80)

    experimental_presets = {k: v for k, v in data.items() if v["type"] == "experimental"}

    # Find longest RT60
    longest_preset = max(experimental_presets.items(), key=lambda x: x[1]["rt60"])
    report_lines.append(f"• Longest RT60: {longest_preset[1]['name']} ({longest_preset[1]['rt60']:.2f}s)")

    # Find shortest RT60
    shortest_preset = min(experimental_presets.items(), key=lambda x: x[1]["rt60"])
    report_lines.append(f"• Shortest RT60: {shortest_preset[1]['name']} ({shortest_preset[1]['rt60']:.2f}s)")

    # Count presets meeting targets
    meeting_targets = sum(1 for v in experimental_presets.values()
                         if v["expected_rt60"][0] <= v["rt60"] <= v["expected_rt60"][1])
    report_lines.append(f"• Presets Meeting Expected Range: {meeting_targets}/{len(experimental_presets)}")

    # RT60 > 20s count
    long_tail_count = sum(1 for v in experimental_presets.values() if v["rt60"] > 20)
    report_lines.append(f"• Presets with RT60 > 20s: {long_tail_count}/{len(experimental_presets)}")

    # RT60 > 10s count
    very_long_count = sum(1 for v in data.values() if v["rt60"] > 10)
    report_lines.append(f"• All Presets with RT60 > 10s: {very_long_count}/{len(data)}")

    report_lines.append("")

    # Save report
    report_path = OUTPUT_DIR / "analysis_report.txt"
    with open(report_path, 'w') as f:
        f.write('\n'.join(report_lines))

    print(f"✓ Saved analysis report: {report_path}")

    # Print to console
    print("\n" + '\n'.join(report_lines))


def main():
    """Main analysis workflow"""
    print("=" * 80)
    print("Monument Reverb - Experimental Presets Analysis")
    print("=" * 80)
    print("")

    # Load data
    print("Loading RT60 data for all presets...")
    data = load_all_rt60_data()
    print(f"✓ Loaded data for {len(data)} presets")
    print("")

    # Create visualizations
    print("Creating visualizations...")
    create_rt60_comparison_plot(data)
    create_rt60_delta_plot(data)
    print("")

    # Generate statistics report
    print("Generating statistics report...")
    create_statistics_report(data)
    print("")

    print("=" * 80)
    print("Analysis Complete!")
    print(f"Results saved to: {OUTPUT_DIR}/")
    print("=" * 80)


if __name__ == "__main__":
    main()
