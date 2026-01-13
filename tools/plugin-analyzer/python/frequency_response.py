#!/usr/bin/env python3
"""
Frequency Response Analysis Tool

Analyzes frequency response of reverb plugins by comparing input and output signals.
Supports both sine sweep and impulse response analysis.

Usage:
    python3 frequency_response.py <input.wav> <output.wav> [--output metrics.json]
    python3 frequency_response.py <impulse_response.wav> --impulse [--output metrics.json]
"""

import argparse
import json
import os
import sys
import tempfile
from datetime import datetime
from pathlib import Path

cache_root = Path(os.environ.get("MPLCONFIGDIR", "")) if os.environ.get("MPLCONFIGDIR") else None
if not cache_root:
    cache_root = Path(tempfile.gettempdir()) / "monument-mplcache"
os.environ.setdefault("MPLBACKEND", "Agg")
try:
    cache_root.mkdir(parents=True, exist_ok=True)
except OSError:
    cache_root = Path(tempfile.gettempdir()) / "monument-mplcache"
    cache_root.mkdir(parents=True, exist_ok=True)
    os.environ["MPLCONFIGDIR"] = str(cache_root)
    os.environ["XDG_CACHE_HOME"] = str(cache_root)
else:
    os.environ.setdefault("MPLCONFIGDIR", str(cache_root))
    os.environ.setdefault("XDG_CACHE_HOME", str(cache_root))

try:
    import numpy as np
    from scipy.io import wavfile
    from scipy import signal as scipy_signal
    import matplotlib.pyplot as plt
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("\nInstall dependencies:")
    print("  pip3 install numpy scipy matplotlib")
    sys.exit(1)

SCHEMA_VERSION = "1.0.0"


def analyze_impulse_response(impulse_file, output_json=None, plot=True):
    """
    Analyze frequency response from an impulse response.

    Args:
        impulse_file: Path to WAV file containing impulse response
        output_json: Optional path to save JSON metrics
        plot: Whether to generate plots

    Returns:
        Dictionary of frequency response metrics
    """
    print(f"\n{'='*60}")
    print("Frequency Response Analysis (Impulse Response)")
    print(f"{'='*60}\n")

    # Load audio
    print(f"▸ Loading impulse response: {impulse_file}")
    sample_rate, audio = wavfile.read(impulse_file)

    # Convert to mono if stereo
    if len(audio.shape) > 1:
        audio = np.mean(audio, axis=1)

    # Normalize to float32
    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    elif audio.dtype == np.int32:
        max_val = np.abs(audio).max()
        if max_val > 2**23:
            audio = audio.astype(np.float32) / (2**23)
        else:
            audio = audio.astype(np.float32) / 2147483648.0
    elif audio.dtype not in [np.float32, np.float64]:
        max_val = np.abs(audio).max()
        if max_val > 0:
            audio = audio.astype(np.float32) / max_val

    print(f"  Sample rate: {sample_rate} Hz")
    duration_seconds = float(len(audio) / sample_rate)
    print(f"  Duration: {duration_seconds:.3f} seconds")
    print(f"  Samples: {len(audio)}")

    # Compute FFT
    print(f"\n▸ Computing frequency response...")

    # Window the impulse response to reduce spectral leakage
    window = scipy_signal.windows.hann(len(audio))
    audio_windowed = audio * window

    # FFT
    fft = np.fft.rfft(audio_windowed)
    freqs = np.fft.rfftfreq(len(audio), 1/sample_rate)

    # Magnitude response in dB
    magnitude_db = 20 * np.log10(np.abs(fft) + 1e-10)

    # Normalize to 0 dB at peak
    magnitude_db = magnitude_db - np.max(magnitude_db)

    # Analyze response in octave bands
    octave_band_defs = [
        ('63', '63Hz', 45, 90),
        ('125', '125Hz', 90, 177),
        ('250', '250Hz', 177, 355),
        ('500', '500Hz', 355, 710),
        ('1000', '1kHz', 710, 1420),
        ('2000', '2kHz', 1420, 2840),
        ('4000', '4kHz', 2840, 5680),
        ('8000', '8kHz', 5680, 11360),
        ('16000', '16kHz', 11360, 20000)
    ]

    band_analysis = {}
    band_analysis_legacy = {}
    octave_band_ranges = {}

    print(f"\n▸ Octave band analysis:")
    for band_key, band_name, f_low, f_high in octave_band_defs:
        octave_band_ranges[band_name] = (f_low, f_high)
        # Find indices for this band
        mask = (freqs >= f_low) & (freqs <= f_high)

        if np.any(mask):
            band_response = magnitude_db[mask]
            avg_db = np.mean(band_response)
            min_db = np.min(band_response)
            max_db = np.max(band_response)
            variation = max_db - min_db
            flatness_band = np.std(band_response)

            band_analysis[band_key] = {
                'gain_db': float(avg_db),
                'flatness_db': float(flatness_band)
            }

            band_analysis_legacy[band_name] = {
                'frequency_range': f'{f_low}-{f_high} Hz',
                'average_db': float(avg_db),
                'min_db': float(min_db),
                'max_db': float(max_db),
                'variation_db': float(variation),
                'flatness_db': float(flatness_band)
            }

            print(f"  {band_name:6s}: {avg_db:6.2f} dB (±{variation/2:.2f} dB)")

    # Overall flatness assessment
    # Calculate variance across the audio spectrum (20Hz-20kHz)
    audio_range_mask = (freqs >= 20) & (freqs <= 20000)
    audio_spectrum = magnitude_db[audio_range_mask]
    flatness_db = np.std(audio_spectrum)
    mean_gain_db = np.mean(audio_spectrum)

    audio_freqs = freqs[audio_range_mask]
    if audio_spectrum.size > 0:
        peak_frequency_hz = float(audio_freqs[np.argmax(audio_spectrum)])
        notch_frequency_hz = float(audio_freqs[np.argmin(audio_spectrum)])
    else:
        peak_frequency_hz = None
        notch_frequency_hz = None

    print(f"\n▸ Overall flatness: ±{flatness_db:.2f} dB std deviation")

    if flatness_db < 3:
        quality_rating = "Excellent"
        flatness_rating = "Excellent (±3dB)"
    elif flatness_db < 6:
        quality_rating = "Good"
        flatness_rating = "Good (±6dB)"
    elif flatness_db < 10:
        quality_rating = "Fair"
        flatness_rating = "Fair (±10dB)"
    else:
        quality_rating = "Colored"
        flatness_rating = "Colored (>±10dB)"

    print(f"  Rating: {flatness_rating}")

    # Build results
    results = {
        'version': SCHEMA_VERSION,
        'timestamp': datetime.utcnow().replace(microsecond=0).isoformat() + 'Z',
        'input_file': str(impulse_file),
        'sample_rate': int(sample_rate),
        'broadband': {
            'flatness_db': float(flatness_db),
            'mean_gain_db': float(mean_gain_db),
            'peak_frequency_hz': peak_frequency_hz,
            'notch_frequency_hz': notch_frequency_hz
        },
        'octave_bands': band_analysis,
        'quality_rating': quality_rating,
        'overall': {
            'flatness_std_db': float(flatness_db),
            'flatness_rating': flatness_rating,
            'frequency_range': '20-20000 Hz',
            'mean_gain_db': float(mean_gain_db)
        },
        'octave_bands_legacy': band_analysis_legacy,
        '_metadata': {
            'audio_file': str(impulse_file),
            'sample_rate': int(sample_rate),
            'duration_seconds': duration_seconds,
            'analysis_method': 'FFT of windowed impulse response'
        }
    }

    # Save to JSON if requested
    if output_json:
        output_path = Path(output_json)
        output_path.parent.mkdir(parents=True, exist_ok=True)

        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(results, f, indent=2)

        print(f"\n▸ Saved metrics to: {output_json}")

    # Generate plot if requested
    if plot:
        try:
            plot_path = Path(impulse_file).with_name(
                Path(impulse_file).stem + '_frequency_response.png'
            )
            generate_frequency_plot(freqs, magnitude_db, octave_band_ranges, plot_path)
            print(f"▸ Saved plot to: {plot_path}")
        except Exception as e:
            print(f"▸ Failed to generate plot: {e}")

    print(f"\n{'='*60}")
    print("✓ Analysis complete")
    print(f"{'='*60}\n")

    return results


def generate_frequency_plot(freqs, magnitude_db, octave_bands, output_path):
    """Generate frequency response plot."""

    fig, ax = plt.subplots(1, 1, figsize=(12, 6))

    # Plot frequency response
    ax.semilogx(freqs, magnitude_db, linewidth=1.0, color='blue', alpha=0.7)

    # Add reference lines
    ax.axhline(0, color='black', linestyle='-', linewidth=0.5)
    ax.axhline(-3, color='gray', linestyle='--', linewidth=0.5, alpha=0.5, label='±3dB')
    ax.axhline(3, color='gray', linestyle='--', linewidth=0.5, alpha=0.5)
    ax.axhline(-6, color='gray', linestyle=':', linewidth=0.5, alpha=0.3, label='±6dB')
    ax.axhline(6, color='gray', linestyle=':', linewidth=0.5, alpha=0.3)

    # Mark octave band centers
    for band_name, (f_low, f_high) in octave_bands.items():
        f_center = np.sqrt(f_low * f_high)
        ax.axvline(f_center, color='red', linestyle=':', linewidth=0.5, alpha=0.2)

    ax.set_xlabel('Frequency (Hz)')
    ax.set_ylabel('Magnitude (dB)')
    ax.set_title('Frequency Response')
    ax.grid(True, which='both', alpha=0.3)
    ax.set_xlim([20, 20000])
    ax.set_ylim([-40, 10])
    ax.legend(loc='upper right')

    plt.tight_layout()
    plt.savefig(output_path, dpi=150)
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Analyze frequency response of reverb plugins',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Analyze impulse response
  python3 frequency_response.py wet.wav --impulse

  # Save metrics to JSON
  python3 frequency_response.py wet.wav --impulse --output metrics.json

  # Skip plot generation
  python3 frequency_response.py wet.wav --impulse --no-plot
        """
    )

    parser.add_argument('input_file', type=Path,
                       help='Path to input WAV file (impulse response or dry signal)')
    parser.add_argument('--impulse', action='store_true',
                       help='Analyze as impulse response (single file)')
    parser.add_argument('--output', '-o', type=Path,
                       help='Output JSON file for metrics')
    parser.add_argument('--no-plot', action='store_true',
                       help='Skip plot generation')

    args = parser.parse_args()

    if not args.input_file.exists():
        print(f"Error: File not found: {args.input_file}")
        return 1

    if not args.impulse:
        print("Error: Only --impulse mode is currently supported")
        print("Usage: python3 frequency_response.py <impulse_response.wav> --impulse")
        return 1

    try:
        analyze_impulse_response(args.input_file, args.output, plot=not args.no_plot)
        return 0
    except Exception as e:
        print(f"\nError during analysis: {e}")
        import traceback
        traceback.print_exc()
        return 1


if __name__ == '__main__':
    sys.exit(main())
