#!/usr/bin/env python3
"""
Audio Numerical Stability Checker
Detects NaN, Inf, denormals, and DC offset in audio output.

Exit Codes:
  0 - All stability checks passed
  1 - One or more stability issues detected
  2 - Invalid input or error

Usage:
  python3 tools/check_audio_stability.py <audio_file.wav>
  python3 tools/check_audio_stability.py test-results/preset-0/wet.wav
  python3 tools/check_audio_stability.py test-results/preset-*/wet.wav  # Check all presets
"""

import sys
import wave
import struct
import math
from pathlib import Path
from typing import List, Tuple, Dict
from datetime import datetime
import numpy as np

# ANSI color codes
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BLUE = '\033[94m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

# Stability thresholds
DENORMAL_THRESHOLD = 1e-38  # Values below this are considered denormals (FLT_MIN ≈ 1.175e-38)
DC_OFFSET_THRESHOLD_DB = -60.0  # DC component should be < -60dB
MAX_ALLOWED_NAN = 0  # Zero tolerance for NaN
MAX_ALLOWED_INF = 0  # Zero tolerance for Inf
MAX_ALLOWED_DENORMALS_PERCENT = 0.01  # Allow up to 0.01% denormals (tail samples)

class AudioStats:
    """Statistics for audio stability analysis."""
    def __init__(self):
        self.total_samples = 0
        self.nan_count = 0
        self.inf_count = 0
        self.denormal_count = 0
        self.dc_offset_db = 0.0
        self.rms_db = 0.0
        self.peak_db = 0.0
        self.denormal_percent = 0.0
        self.violations = []

def load_audio_file(file_path: Path) -> Tuple[np.ndarray, int]:
    """
    Load audio file and return samples + sample rate.

    Returns:
        (samples, sample_rate) tuple where samples is float32 array
    """
    try:
        with wave.open(str(file_path), 'rb') as wf:
            num_channels = wf.getnchannels()
            sample_width = wf.getsampwidth()
            sample_rate = wf.getframerate()
            num_frames = wf.getnframes()

            # Read raw audio data
            raw_data = wf.readframes(num_frames)

            # Convert to numpy array based on sample width
            if sample_width == 2:  # 16-bit PCM
                samples = np.frombuffer(raw_data, dtype=np.int16)
                samples = samples.astype(np.float32) / 32768.0
            elif sample_width == 3:  # 24-bit PCM
                # Unpack 24-bit samples manually
                samples = []
                for i in range(0, len(raw_data), 3):
                    # Little-endian 24-bit signed
                    val = int.from_bytes(raw_data[i:i+3], byteorder='little', signed=True)
                    samples.append(val / 8388608.0)  # 2^23
                samples = np.array(samples, dtype=np.float32)
            elif sample_width == 4:  # 32-bit float or PCM
                samples = np.frombuffer(raw_data, dtype=np.float32)
            else:
                raise ValueError(f"Unsupported sample width: {sample_width}")

            # Handle multi-channel (interleaved)
            if num_channels > 1:
                samples = samples.reshape(-1, num_channels)
                # Use L+R average for stereo analysis
                samples = np.mean(samples, axis=1)

            return samples, sample_rate

    except FileNotFoundError:
        print(f"{Colors.RED}✗ Error: Audio file not found: {file_path}{Colors.RESET}")
        sys.exit(2)
    except Exception as e:
        print(f"{Colors.RED}✗ Error loading audio file: {e}{Colors.RESET}")
        sys.exit(2)

def analyze_audio_stability(samples: np.ndarray) -> AudioStats:
    """
    Analyze audio samples for numerical stability issues.

    Checks for:
    - NaN (Not a Number) values
    - Inf (Infinity) values
    - Denormal numbers (very small values)
    - DC offset (bias away from zero)
    """
    stats = AudioStats()
    stats.total_samples = len(samples)

    # Check for NaN
    stats.nan_count = np.sum(np.isnan(samples))

    # Check for Inf
    stats.inf_count = np.sum(np.isinf(samples))

    # Check for denormals (very small values)
    abs_samples = np.abs(samples)
    non_zero_mask = abs_samples > 0.0  # Exclude actual zeros
    stats.denormal_count = np.sum((abs_samples > 0.0) & (abs_samples < DENORMAL_THRESHOLD))
    stats.denormal_percent = (stats.denormal_count / stats.total_samples) * 100.0

    # Calculate DC offset
    dc_offset = np.mean(samples)
    dc_offset_linear = abs(dc_offset)
    stats.dc_offset_db = 20.0 * math.log10(dc_offset_linear + 1e-10)  # Avoid log(0)

    # Calculate RMS and peak (for context)
    rms = np.sqrt(np.mean(samples ** 2))
    stats.rms_db = 20.0 * math.log10(rms + 1e-10)
    peak = np.max(abs_samples)
    stats.peak_db = 20.0 * math.log10(peak + 1e-10)

    return stats

def check_stability_thresholds(stats: AudioStats) -> bool:
    """
    Check if audio meets stability requirements.

    Returns:
        True if all checks passed, False otherwise
    """
    all_passed = True

    # Check NaN
    if stats.nan_count > MAX_ALLOWED_NAN:
        stats.violations.append(f"NaN detected: {stats.nan_count} samples contain NaN")
        all_passed = False

    # Check Inf
    if stats.inf_count > MAX_ALLOWED_INF:
        stats.violations.append(f"Inf detected: {stats.inf_count} samples contain Inf")
        all_passed = False

    # Check denormals (allow small percentage for tail samples)
    if stats.denormal_percent > MAX_ALLOWED_DENORMALS_PERCENT:
        stats.violations.append(
            f"Excessive denormals: {stats.denormal_percent:.3f}% exceeds "
            f"threshold {MAX_ALLOWED_DENORMALS_PERCENT:.3f}%"
        )
        all_passed = False

    # Check DC offset
    if stats.dc_offset_db > DC_OFFSET_THRESHOLD_DB:
        stats.violations.append(
            f"DC offset too high: {stats.dc_offset_db:.1f} dB exceeds "
            f"threshold {DC_OFFSET_THRESHOLD_DB:.1f} dB"
        )
        all_passed = False

    return all_passed

def print_results(file_path: Path, stats: AudioStats, passed: bool):
    """Print stability check results."""
    print(f"\n{Colors.BOLD}{'=' * 80}{Colors.RESET}")
    print(f"{Colors.BOLD}NUMERICAL STABILITY CHECK{Colors.RESET}")
    print(f"{Colors.BOLD}{'=' * 80}{Colors.RESET}")

    print(f"\n{Colors.BOLD}File: {file_path}{Colors.RESET}")
    print(f"Total samples: {stats.total_samples:,}")
    print(f"Duration: {stats.total_samples / 48000.0:.2f}s (@ 48kHz)")

    print(f"\n{Colors.BOLD}Stability Checks:{Colors.RESET}")
    print(f"{'Check':<30} {'Value':<20} {'Threshold':<20} {'Status':<10}")
    print("-" * 90)

    # NaN check
    nan_status = f"{Colors.GREEN}✓ PASS{Colors.RESET}" if stats.nan_count == 0 else f"{Colors.RED}✗ FAIL{Colors.RESET}"
    print(f"{'NaN samples':<30} {stats.nan_count:<20} {'= 0':<20} {nan_status}")

    # Inf check
    inf_status = f"{Colors.GREEN}✓ PASS{Colors.RESET}" if stats.inf_count == 0 else f"{Colors.RED}✗ FAIL{Colors.RESET}"
    print(f"{'Inf samples':<30} {stats.inf_count:<20} {'= 0':<20} {inf_status}")

    # Denormal check
    denormal_status = (
        f"{Colors.GREEN}✓ PASS{Colors.RESET}"
        if stats.denormal_percent <= MAX_ALLOWED_DENORMALS_PERCENT
        else f"{Colors.RED}✗ FAIL{Colors.RESET}"
    )
    print(f"{'Denormal samples':<30} "
          f"{f'{stats.denormal_count} ({stats.denormal_percent:.3f}%)':<20} "
          f"{f'<= {MAX_ALLOWED_DENORMALS_PERCENT:.3f}%':<20} {denormal_status}")

    # DC offset check
    dc_status = (
        f"{Colors.GREEN}✓ PASS{Colors.RESET}"
        if stats.dc_offset_db <= DC_OFFSET_THRESHOLD_DB
        else f"{Colors.RED}✗ FAIL{Colors.RESET}"
    )
    print(f"{'DC offset':<30} {f'{stats.dc_offset_db:.1f} dB':<20} "
          f"{f'<= {DC_OFFSET_THRESHOLD_DB:.1f} dB':<20} {dc_status}")

    print(f"\n{Colors.BOLD}Audio Statistics (informational):{Colors.RESET}")
    print(f"  RMS level: {stats.rms_db:.1f} dB")
    print(f"  Peak level: {stats.peak_db:.1f} dB")

    if passed:
        print(f"\n{Colors.GREEN}{Colors.BOLD}✓ ALL STABILITY CHECKS PASSED{Colors.RESET}")
        print(f"{Colors.GREEN}Audio output is numerically stable.{Colors.RESET}")
    else:
        print(f"\n{Colors.RED}{Colors.BOLD}✗ STABILITY VIOLATIONS DETECTED{Colors.RESET}")
        print(f"\n{Colors.RED}Violations ({len(stats.violations)}):{Colors.RESET}")
        for i, violation in enumerate(stats.violations, 1):
            print(f"  {i}. {violation}")

        print(f"\n{Colors.YELLOW}Recommended Actions:{Colors.RESET}")
        if stats.nan_count > 0:
            print(f"  - NaN indicates division by zero or invalid math operations")
            print(f"  - Check filter coefficient calculations")
            print(f"  - Verify parameter range validation")
        if stats.inf_count > 0:
            print(f"  - Inf indicates overflow or division by very small number")
            print(f"  - Add safety clamps to gain stages")
            print(f"  - Check feedback loop stability")
        if stats.denormal_percent > MAX_ALLOWED_DENORMALS_PERCENT:
            print(f"  - Denormals degrade CPU performance significantly")
            print(f"  - Use juce::ScopedNoDenormals in processBlock")
            print(f"  - Add DC blocker if needed")
        if stats.dc_offset_db > DC_OFFSET_THRESHOLD_DB:
            print(f"  - DC offset can cause clicks and pops")
            print(f"  - Add high-pass filter (1-5 Hz cutoff)")
            print(f"  - Check for uninitialized state variables")

    print(f"\n{Colors.BOLD}{'=' * 80}{Colors.RESET}")

def main():
    """Main entry point for audio stability checker."""
    if len(sys.argv) != 2:
        print(f"{Colors.RED}Usage: python3 {sys.argv[0]} <audio_file.wav>{Colors.RESET}")
        print(f"\nExample:")
        print(f"  python3 {sys.argv[0]} test-results/preset-0/wet.wav")
        sys.exit(2)

    file_path = Path(sys.argv[1])

    print(f"{Colors.BOLD}Monument Reverb - Audio Stability Checker{Colors.RESET}")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    # Load audio file
    samples, sample_rate = load_audio_file(file_path)

    # Analyze stability
    stats = analyze_audio_stability(samples)

    # Check thresholds
    passed = check_stability_thresholds(stats)

    # Print results
    print_results(file_path, stats, passed)

    # Exit with appropriate code
    sys.exit(0 if passed else 1)

if __name__ == "__main__":
    main()
