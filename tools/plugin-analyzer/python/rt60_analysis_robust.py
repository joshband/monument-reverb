#!/usr/bin/env python3
"""
Enhanced RT60 Analysis Tool with Robust Decay Detection

Analyzes reverb impulse responses with fallback methods when
pyroomacoustics fails.

Usage:
    python3 rt60_analysis_robust.py <impulse_response.wav> [--output metrics.json]
"""

import argparse
import json
import sys
from pathlib import Path

try:
    import numpy as np
    import scipy.io.wavfile as wavfile
    import scipy.signal
    import matplotlib.pyplot as plt
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("\nInstall dependencies:")
    print("  pip3 install numpy scipy matplotlib")
    sys.exit(1)

# Try to import pyroomacoustics, but it's optional
try:
    import pyroomacoustics as pra
    HAS_PRA = True
except ImportError:
    HAS_PRA = False
    print("Note: pyroomacoustics not available, using manual RT60 calculation")


def manual_rt60_calculation(audio, sample_rate, decay_db=-60):
    """
    Manual RT60 calculation using Schroeder backward integration.

    This is a fallback when pyroomacoustics fails or isn't available.
    """
    # Square the signal to get energy
    energy = audio ** 2

    # Check if there's any energy
    max_energy = np.max(energy)
    if max_energy < 1e-10:
        return None, "Signal has insufficient energy"

    # Schroeder backward integration
    # Start from the end and integrate backwards
    energy_cumsum = np.cumsum(energy[::-1])[::-1]

    # Convert to dB
    energy_db = 10 * np.log10(energy_cumsum + 1e-10)

    # Normalize to 0 dB at peak
    energy_db = energy_db - np.max(energy_db)

    # Find where it crosses the decay_db threshold
    crossings = np.where(energy_db <= decay_db)[0]

    if len(crossings) == 0:
        return None, f"Signal doesn't decay {decay_db} dB within the recording"

    # RT60 is the time to decay 60 dB
    decay_time = crossings[0] / sample_rate

    # If we're measuring decay other than -60dB, scale it
    rt60 = decay_time * (60.0 / abs(decay_db))

    return rt60, energy_db


def analyze_signal_quality(audio, sample_rate):
    """Analyze the signal to understand why RT60 might fail."""

    print("\n▸ Signal Quality Check:")

    # Peak amplitude
    peak = np.max(np.abs(audio))
    print(f"  Peak amplitude: {peak:.6f} ({20*np.log10(peak+1e-10):.1f} dBFS)")

    # RMS level
    rms = np.sqrt(np.mean(audio ** 2))
    print(f"  RMS level: {rms:.6f} ({20*np.log10(rms+1e-10):.1f} dBFS)")

    # Energy distribution (first 10% vs last 10%)
    split_point = len(audio) // 10
    early_energy = np.sum(audio[:split_point] ** 2)
    late_energy = np.sum(audio[-split_point:] ** 2)

    if early_energy > 0:
        energy_ratio_db = 10 * np.log10((early_energy + 1e-10) / (late_energy + 1e-10))
        print(f"  Energy decay: {energy_ratio_db:.1f} dB (early vs late 10%)")

    # Estimate usable decay range
    energy = audio ** 2
    energy_cumsum = np.cumsum(energy[::-1])[::-1]
    energy_db = 10 * np.log10(energy_cumsum + 1e-10)
    energy_db = energy_db - np.max(energy_db)

    min_db = np.min(energy_db[energy_db > -np.inf])
    print(f"  Dynamic range: {min_db:.1f} dB")
    print(f"  Noise floor at: ~{min_db:.0f} dB")

    return {
        'peak': float(peak),
        'rms': float(rms),
        'dynamic_range_db': float(min_db)
    }


def analyze_rt60(audio_file, output_json=None, plot=True):
    """
    Analyze RT60 from an impulse response WAV file.
    """
    print(f"\n{'='*60}")
    print("Enhanced RT60 Analysis")
    print(f"{'='*60}\n")

    # Load audio
    print(f"▸ Loading impulse response: {audio_file}")
    sample_rate, audio = wavfile.read(audio_file)

    # Convert to mono if stereo (average channels)
    if len(audio.shape) > 1:
        audio = np.mean(audio, axis=1)

    # Normalize to float32
    # Note: 24-bit WAV files are read as int32 by scipy, but only use lower 24 bits
    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    elif audio.dtype == np.int32:
        # Check if this is actually 24-bit audio (common for reverb IRs)
        max_val = np.abs(audio).max()
        if max_val > 2**23:  # Likely 24-bit stored as int32
            audio = audio.astype(np.float32) / (2**23)  # 24-bit: 8388608
        else:
            audio = audio.astype(np.float32) / 2147483648.0  # True 32-bit
    elif audio.dtype not in [np.float32, np.float64]:
        # Use max value for normalization
        max_val = np.abs(audio).max()
        if max_val > 0:
            audio = audio.astype(np.float32) / max_val

    print(f"  Sample rate: {sample_rate} Hz")
    print(f"  Duration: {len(audio) / sample_rate:.3f} seconds")
    print(f"  Samples: {len(audio)}")

    # Check signal quality
    quality_metrics = analyze_signal_quality(audio, sample_rate)

    # Try pyroomacoustics first if available
    rt60_broadband = None
    method = "manual"

    if HAS_PRA:
        try:
            print("\n▸ Attempting pyroomacoustics RT60 measurement...")
            rt60_broadband = pra.experimental.rt60.measure_rt60(audio, fs=sample_rate, decay_db=60)
            if rt60_broadband is not None and not np.isnan(rt60_broadband) and rt60_broadband > 0:
                print(f"  ✓ Success: {rt60_broadband:.3f} seconds")
                method = "pyroomacoustics"
            else:
                raise ValueError("pyroomacoustics returned invalid RT60")
        except Exception as e:
            print(f"  ✗ Failed: {e}")
            print("  Falling back to manual calculation...")
            rt60_broadband = None

    # Fallback to manual calculation
    if rt60_broadband is None:
        print("\n▸ Manual RT60 calculation...")
        rt60_broadband, energy_db = manual_rt60_calculation(audio, sample_rate, decay_db=-60)

        if rt60_broadband is not None:
            print(f"  ✓ RT60: {rt60_broadband:.3f} seconds")
            print(f"  Method: Schroeder backward integration")
        else:
            print(f"  ✗ Failed: {energy_db}")

            # Try with less strict decay requirement
            print("\n▸ Trying with -30dB decay threshold...")
            rt30, energy_db = manual_rt60_calculation(audio, sample_rate, decay_db=-30)
            if rt30 is not None:
                print(f"  ✓ RT30: {rt30:.3f} seconds (extrapolated to RT60: {rt30*2:.3f}s)")
                rt60_broadband = rt30 * 2  # Extrapolate
                method = "manual_RT30_extrapolated"
    else:
        # Get energy curve for plotting
        _, energy_db = manual_rt60_calculation(audio, sample_rate, decay_db=-60)

    # Build results
    rt60_results = {
        'broadband': {
            'rt60_seconds': float(rt60_broadband) if rt60_broadband is not None else None,
            'frequency_range': '20-20000 Hz',
            'method': method
        },
        '_metadata': {
            'audio_file': str(audio_file),
            'sample_rate': int(sample_rate),
            'duration_seconds': float(len(audio) / sample_rate),
            'analysis_method': method,
            **quality_metrics
        }
    }

    # Save to JSON if requested
    if output_json:
        output_path = Path(output_json)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w') as f:
            json.dump(rt60_results, f, indent=2)

        print(f"\n▸ Saved metrics to: {output_json}")

    # Generate plot if requested
    if plot and energy_db is not None:
        try:
            plot_path = Path(audio_file).with_suffix('.png')
            generate_rt60_plot(audio, sample_rate, rt60_results, energy_db, plot_path)
            print(f"▸ Saved plot to: {plot_path}")
        except Exception as e:
            print(f"▸ Failed to generate plot: {e}")
            import traceback
            traceback.print_exc()

    print(f"\n{'='*60}")
    if rt60_broadband is not None:
        print("✓ Analysis complete")
        print(f"  RT60: {rt60_broadband:.3f} seconds")
    else:
        print("⚠ Analysis completed with warnings")
        print("  Could not determine RT60 - signal may be too short or dry")
    print(f"{'='*60}\n")

    return rt60_results


def generate_rt60_plot(audio, sample_rate, rt60_results, energy_db, output_path):
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
    axes[1].plot(time, energy_db, linewidth=1.0, color='blue', alpha=0.7)
    axes[1].set_xlabel('Time (seconds)')
    axes[1].set_ylabel('Energy (dB)')
    axes[1].set_title('Energy Decay Curve (Schroeder Integration)')
    axes[1].grid(True, alpha=0.3)
    axes[1].set_ylim([max(-80, np.min(energy_db) - 5), 5])

    # Add RT60 markers
    if 'broadband' in rt60_results and rt60_results['broadband']['rt60_seconds']:
        rt60 = rt60_results['broadband']['rt60_seconds']
        if rt60 < time[-1]:  # Only draw if within plot range
            axes[1].axvline(rt60, color='red', linestyle='--', linewidth=2,
                           label=f'RT60 = {rt60:.3f}s')
            axes[1].axhline(-60, color='gray', linestyle=':', linewidth=1, alpha=0.5)
            axes[1].legend()

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Analyze RT60 from reverb impulse response (enhanced version)',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze impulse response
  python3 rt60_analysis_robust.py wet.wav

  # Save metrics to JSON
  python3 rt60_analysis_robust.py wet.wav --output metrics.json

  # Skip plot generation
  python3 rt60_analysis_robust.py wet.wav --no-plot
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
