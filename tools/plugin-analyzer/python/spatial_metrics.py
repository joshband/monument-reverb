#!/usr/bin/env python3
"""
Spatial Metrics Analysis Tool

Computes interaural time difference (ITD), interaural level difference (ILD),
and interaural cross-correlation (IACC) from stereo audio files.

Usage:
    python3 spatial_metrics.py <input.wav> --output spatial_metrics.json
"""

import argparse
import json
import sys
from datetime import datetime
from pathlib import Path

try:
    import numpy as np
    from scipy.io import wavfile
    from scipy import signal as scipy_signal
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("Install dependencies: pip3 install numpy scipy")
    sys.exit(1)

SCHEMA_VERSION = "1.0.0"
EPS = 1e-12
DEFAULT_WINDOW_MS = 80.0
DEFAULT_MAX_LAG_MS = 1.0

OCTAVE_BANDS = [
    ("63", 45, 90),
    ("125", 90, 177),
    ("250", 177, 355),
    ("500", 355, 710),
    ("1000", 710, 1420),
    ("2000", 1420, 2840),
    ("4000", 2840, 5680),
    ("8000", 5680, 11360),
    ("16000", 11360, 20000),
]


def load_audio(path):
    sample_rate, audio = wavfile.read(path)
    notes = []

    if audio.ndim == 1:
        audio = audio[:, np.newaxis]

    if audio.dtype == np.int16:
        audio = audio.astype(np.float32) / 32768.0
    elif audio.dtype == np.int32:
        max_val = np.abs(audio).max()
        if max_val > 2**23:
            audio = audio.astype(np.float32) / (2**23)
        else:
            audio = audio.astype(np.float32) / 2147483648.0
    elif audio.dtype in (np.float32, np.float64):
        audio = audio.astype(np.float32)
    else:
        max_val = np.abs(audio).max()
        if max_val > 0:
            audio = audio.astype(np.float32) / max_val
        else:
            audio = audio.astype(np.float32)

    if audio.shape[1] < 2:
        notes.append("Input is mono; duplicating channel for analysis.")
        audio = np.repeat(audio, 2, axis=1)
    elif audio.shape[1] > 2:
        notes.append("Input has more than 2 channels; using first two.")
        audio = audio[:, :2]

    return sample_rate, audio, notes


def select_window(audio, sample_rate, window_ms):
    peak_index = int(np.argmax(np.max(np.abs(audio), axis=1)))
    window_samples = int(sample_rate * window_ms / 1000.0)
    if window_samples <= 0:
        window_samples = audio.shape[0]
    end = min(audio.shape[0], peak_index + window_samples)
    if end <= peak_index:
        peak_index = 0
        end = audio.shape[0]
    return peak_index, end, window_samples


def compute_rms(signal):
    return float(np.sqrt(np.mean(signal * signal) + EPS))


def compute_ild_db(left, right):
    rms_left = compute_rms(left)
    rms_right = compute_rms(right)
    ild_db = 20.0 * np.log10((rms_left + EPS) / (rms_right + EPS))
    return float(ild_db), rms_left, rms_right


def compute_band_ild(left, right, sample_rate):
    length = len(left)
    window = np.hanning(length)
    left_fft = np.fft.rfft(left * window)
    right_fft = np.fft.rfft(right * window)
    freqs = np.fft.rfftfreq(length, d=1.0 / sample_rate)

    left_power = np.abs(left_fft) ** 2
    right_power = np.abs(right_fft) ** 2

    band_results = {}
    for band_key, f_low, f_high in OCTAVE_BANDS:
        mask = (freqs >= f_low) & (freqs <= f_high)
        if not np.any(mask):
            continue
        energy_left = np.sum(left_power[mask])
        energy_right = np.sum(right_power[mask])
        ild_db = 10.0 * np.log10((energy_left + EPS) / (energy_right + EPS))
        band_results[band_key] = {"ild_db": float(ild_db)}

    return band_results


def normalized_cross_correlation(left, right):
    left = left - np.mean(left)
    right = right - np.mean(right)
    corr = scipy_signal.correlate(left, right, mode="full", method="fft")
    denom = np.sqrt(np.sum(left ** 2) * np.sum(right ** 2)) + EPS
    corr = corr / denom
    lags = np.arange(-len(left) + 1, len(left))
    return corr, lags


def gcc_phat(sig, refsig, sample_rate, max_lag_samples):
    n = sig.shape[0] + refsig.shape[0]
    sig_fft = np.fft.rfft(sig, n=n)
    ref_fft = np.fft.rfft(refsig, n=n)
    cross_power = sig_fft * np.conj(ref_fft)
    denom = np.abs(cross_power)
    denom[denom < EPS] = EPS
    cross_power /= denom

    corr = np.fft.irfft(cross_power, n=n)
    max_shift = int(max_lag_samples)
    if max_shift <= 0:
        max_shift = n // 2
    max_shift = min(max_shift, n // 2)
    corr = np.concatenate((corr[-max_shift:], corr[:max_shift + 1]))
    shift = int(np.argmax(np.abs(corr)) - max_shift)
    tau = shift / float(sample_rate)
    return tau, shift


def analyze_spatial_metrics(audio, sample_rate, window_ms, max_lag_ms):
    notes = []
    start, end, requested_window_samples = select_window(audio, sample_rate, window_ms)
    if end - start < requested_window_samples:
        notes.append("Analysis window truncated due to file length.")

    left = audio[start:end, 0]
    right = audio[start:end, 1]

    max_lag_samples = int(sample_rate * max_lag_ms / 1000.0)
    if max_lag_samples < 1:
        max_lag_samples = 1
        notes.append("Max lag too small; clamped to 1 sample.")

    ild_db, rms_left, rms_right = compute_ild_db(left, right)
    band_ild = compute_band_ild(left, right, sample_rate)

    itd_seconds, itd_samples = gcc_phat(left, right, sample_rate, max_lag_samples)

    corr, lags = normalized_cross_correlation(left, right)
    lag_mask = (lags >= -max_lag_samples) & (lags <= max_lag_samples)
    corr_window = corr[lag_mask]
    lags_window = lags[lag_mask]

    peak_index = int(np.argmax(np.abs(corr_window)))
    iacc_signed_raw = float(corr_window[peak_index])
    iacc_signed = float(np.clip(iacc_signed_raw, -1.0, 1.0))
    iacc_lag_samples = int(lags_window[peak_index])

    zero_lag_index = np.where(lags == 0)[0]
    corr_zero_lag_raw = float(corr[zero_lag_index[0]]) if zero_lag_index.size else 0.0
    corr_zero_lag = float(np.clip(corr_zero_lag_raw, -1.0, 1.0))

    results = {
        "version": SCHEMA_VERSION,
        "timestamp": datetime.utcnow().replace(microsecond=0).isoformat() + "Z",
        "input_file": "",
        "sample_rate": int(sample_rate),
        "num_channels": int(audio.shape[1]),
        "analysis_window": {
            "start_sample": int(start),
            "length_samples": int(end - start),
            "length_ms": float((end - start) * 1000.0 / sample_rate),
            "requested_length_samples": int(requested_window_samples),
            "max_lag_samples": int(max_lag_samples),
            "max_lag_ms": float(max_lag_ms),
        },
        "broadband": {
            "itd_seconds": float(itd_seconds),
            "itd_samples": int(itd_samples),
            "ild_db": float(ild_db),
            "iacc": float(np.clip(abs(iacc_signed), 0.0, 1.0)),
            "iacc_signed": float(iacc_signed),
            "iacc_lag_samples": int(iacc_lag_samples),
            "corr_zero_lag": float(corr_zero_lag),
            "rms_left": float(rms_left),
            "rms_right": float(rms_right),
        },
        "octave_bands": band_ild,
        "_metadata": {
            "analysis_method": {
                "itd": "gcc_phat",
                "iacc": "normalized_cross_correlation",
                "ild": "rms_ratio",
                "band_ild": "fft_band_energy",
            },
            "window_strategy": "peak_onset",
        },
    }

    if notes:
        results["analysis_notes"] = "; ".join(notes)

    return results


def main():
    parser = argparse.ArgumentParser(
        description="Analyze spatial cues (ITD/ILD/IACC) from stereo audio",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  python3 spatial_metrics.py wet.wav --output spatial_metrics.json
  python3 spatial_metrics.py wet.wav --window-ms 120 --max-lag-ms 2.0
        """,
    )

    parser.add_argument("input_file", type=Path, help="Path to input WAV file")
    parser.add_argument("--output", "-o", type=Path, help="Output JSON file for metrics")
    parser.add_argument("--window-ms", type=float, default=DEFAULT_WINDOW_MS,
                        help=f"Early-window length in ms (default: {DEFAULT_WINDOW_MS})")
    parser.add_argument("--max-lag-ms", type=float, default=DEFAULT_MAX_LAG_MS,
                        help=f"Max ITD lag in ms (default: {DEFAULT_MAX_LAG_MS})")

    args = parser.parse_args()

    if not args.input_file.exists():
        print(f"Error: File not found: {args.input_file}")
        return 1

    sample_rate, audio, notes = load_audio(args.input_file)
    results = analyze_spatial_metrics(audio, sample_rate, args.window_ms, args.max_lag_ms)
    results["input_file"] = str(args.input_file)

    if notes:
        existing = results.get("analysis_notes", "")
        combined = "; ".join([existing, *notes]) if existing else "; ".join(notes)
        results["analysis_notes"] = combined

    print("Spatial Metrics Analysis")
    print("========================")
    print(f"File: {args.input_file}")
    print(f"Sample rate: {sample_rate} Hz")
    print(f"Window: {results['analysis_window']['length_ms']:.1f} ms")
    print(f"ITD: {results['broadband']['itd_seconds'] * 1000.0:.3f} ms")
    print(f"ILD: {results['broadband']['ild_db']:.2f} dB")
    print(f"IACC: {results['broadband']['iacc']:.3f}")
    if "analysis_notes" in results:
        print(f"Notes: {results['analysis_notes']}")

    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        with open(args.output, "w") as handle:
            json.dump(results, handle, indent=2)
        print(f"Saved metrics: {args.output}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
