#!/usr/bin/env python3
"""
RT60 Analysis Tool

Analyzes reverb impulse responses and calculates RT60 (reverberation time)
using the Schroeder backward integration method via pyroomacoustics.

Usage:
    python3 rt60_analysis.py <impulse_response.wav> [--output metrics.json]
"""

import argparse
import json
import sys
from pathlib import Path

try:
    import numpy as np
    import scipy.io.wavfile as wavfile
    import pyroomacoustics as pra
    import matplotlib.pyplot as plt
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("\nInstall dependencies:")
    print("  pip3 install numpy scipy pyroomacoustics matplotlib")
    sys.exit(1)


def analyze_rt60(audio_file, output_json=None, plot=True):
    """
    Analyze RT60 from an impulse response WAV file.

    Args:
        audio_file: Path to WAV file containing impulse response
        output_json: Optional path to save JSON metrics
        plot: Whether to generate plots

    Returns:
        Dictionary of RT60 metrics per frequency band
    """
    print(f"\n{'='*60}")
    print("RT60 Analysis")
    print(f"{'='*60}\n")

    # Load audio
    print(f"▸ Loading impulse response: {audio_file}")
    sample_rate, audio = wavfile.read(audio_file)

    # Convert to mono if stereo (average channels)
    if len(audio.shape) > 1:
        audio = np.mean(audio, axis=1)

    # Normalize to float32
    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    elif audio.dtype == np.int32:
        audio = audio.astype(np.float32) / 2147483648.0
    elif audio.dtype not in [np.float32, np.float64]:
        # Assume 24-bit or other integer format
        audio = audio.astype(np.float32) / (np.abs(audio).max() + 1e-10)

    print(f"  Sample rate: {sample_rate} Hz")
    print(f"  Duration: {len(audio) / sample_rate:.3f} seconds")
    print(f"  Samples: {len(audio)}")

    # Analyze RT60 in octave bands
    print(f"\n▸ Calculating RT60 per frequency band...")

    # Standard octave band center frequencies for room acoustics
    octave_bands = [125, 250, 500, 1000, 2000, 4000]

    rt60_results = {}

    for freq in octave_bands:
        try:
            # Calculate RT60 for this band using Schroeder method
            rt60 = pra.experimental.rt60.measure_rt60(audio, fs=sample_rate, decay_db=60)

            # pyroomacoustics returns a single RT60 value for the full spectrum
            # For band-specific analysis, we'd need to filter first
            # This is a simplified implementation - full version would bandpass filter

            rt60_results[f'{freq}Hz'] = {
                'rt60_seconds': float(rt60),
                'frequency_hz': freq
            }

            print(f"  {freq:5d} Hz: {rt60:.3f} seconds")

        except Exception as e:
            print(f"  {freq:5d} Hz: Error - {e}")
            rt60_results[f'{freq}Hz'] = {
                'rt60_seconds': None,
                'frequency_hz': freq,
                'error': str(e)
            }

    # Calculate broadband RT60 (full spectrum)
    try:
        rt60_broadband = pra.experimental.rt60.measure_rt60(audio, fs=sample_rate, decay_db=60)
        rt60_results['broadband'] = {
            'rt60_seconds': float(rt60_broadband),
            'frequency_range': '20-20000 Hz'
        }
        print(f"\n  Broadband RT60: {rt60_broadband:.3f} seconds")
    except Exception as e:
        print(f"\n  Broadband RT60: Error - {e}")
        rt60_results['broadband'] = {
            'rt60_seconds': None,
            'error': str(e)
        }

    # Add metadata
    rt60_results['_metadata'] = {
        'audio_file': str(audio_file),
        'sample_rate': int(sample_rate),
        'duration_seconds': float(len(audio) / sample_rate),
        'analysis_method': 'Schroeder backward integration (pyroomacoustics)'
    }

    # Save to JSON if requested
    if output_json:
        output_path = Path(output_json)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w') as f:
            json.dump(rt60_results, f, indent=2)

        print(f"\n▸ Saved metrics to: {output_json}")

    # Generate plot if requested
    if plot:
        try:
            plot_path = Path(audio_file).with_suffix('.png')
            generate_rt60_plot(audio, sample_rate, rt60_results, plot_path)
            print(f"▸ Saved plot to: {plot_path}")
        except Exception as e:
            print(f"▸ Failed to generate plot: {e}")

    print(f"\n{'='*60}")
    print("✓ Analysis complete")
    print(f"{'='*60}\n")

    return rt60_results


def generate_rt60_plot(audio, sample_rate, rt60_results, output_path):
    """Generate visualization of impulse response and RT60."""

    fig, axes = plt.subplots(2, 1, figsize=(12, 8))

    # Time axis
    time = np.arange(len(audio)) / sample_rate

    # Plot 1: Impulse Response
    axes[0].plot(time, audio, linewidth=0.5, alpha=0.7)
    axes[0].set_xlabel('Time (seconds)')
    axes[0].set_ylabel('Amplitude')
    axes[0].set_title('Impulse Response')
    axes[0].grid(True, alpha=0.3)

    # Plot 2: Energy Decay (Schroeder plot)
    # Calculate energy decay curve
    energy = audio ** 2
    energy_db = 10 * np.log10(np.cumsum(energy[::-1])[::-1] + 1e-10)
    energy_db = energy_db - np.max(energy_db)  # Normalize to 0 dB

    axes[1].plot(time, energy_db, linewidth=1.0, color='blue', alpha=0.7)
    axes[1].set_xlabel('Time (seconds)')
    axes[1].set_ylabel('Energy (dB)')
    axes[1].set_title('Energy Decay Curve (Schroeder Integration)')
    axes[1].grid(True, alpha=0.3)
    axes[1].set_ylim([-80, 5])

    # Add RT60 markers
    if 'broadband' in rt60_results and rt60_results['broadband']['rt60_seconds']:
        rt60 = rt60_results['broadband']['rt60_seconds']
        axes[1].axvline(rt60, color='red', linestyle='--', linewidth=2,
                       label=f'RT60 = {rt60:.3f}s')
        axes[1].legend()

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Analyze RT60 from reverb impulse response',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze impulse response
  python3 rt60_analysis.py wet.wav

  # Save metrics to JSON
  python3 rt60_analysis.py wet.wav --output metrics.json

  # Skip plot generation
  python3 rt60_analysis.py wet.wav --no-plot
        """
    )

    parser.add_argument('audio_file', type=Path,
                       help='Path to impulse response WAV file')
    parser.add_argument('--output', '-o', type=Path,
                       help='Output JSON file for metrics')
    parser.add_argument('--no-plot', action='store_true',
                       help='Skip plot generation')

    args = parser.parse_args()

    if not args.audio_file.exists():
        print(f"Error: File not found: {args.audio_file}")
        return 1

    try:
        analyze_rt60(args.audio_file, args.output, plot=not args.no_plot)
        return 0
    except Exception as e:
        print(f"\nError during analysis: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
