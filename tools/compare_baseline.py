#!/usr/bin/env python3
"""
Monument Reverb - Baseline Comparison Tool

Compares current preset captures against baseline reference data
to detect regressions in audio quality or DSP behavior.

Usage:
    python3 compare_baseline.py <baseline_dir> <current_dir> [--threshold 0.1]
"""

import argparse
import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple

try:
    import numpy as np
    from scipy.io import wavfile
    from scipy import signal
except ImportError as e:
    print(f"Error: Missing required Python package: {e}")
    print("\nInstall dependencies:")
    print("  pip3 install numpy scipy")
    sys.exit(1)


DEFAULT_SPATIAL_ITD_MS = 0.2
DEFAULT_SPATIAL_ILD_DB = 1.0
DEFAULT_SPATIAL_IACC_DELTA = 0.05


def load_metrics(preset_dir: Path) -> Dict:
    """Load RT60, frequency response, and spatial metrics for a preset."""
    metrics = {}

    rt60_file = preset_dir / "rt60_metrics.json"
    if rt60_file.exists():
        with open(rt60_file) as f:
            metrics['rt60'] = json.load(f)

    freq_file = preset_dir / "freq_metrics.json"
    if freq_file.exists():
        with open(freq_file) as f:
            metrics['freq_response'] = json.load(f)

    spatial_file = preset_dir / "spatial_metrics.json"
    if spatial_file.exists():
        with open(spatial_file) as f:
            metrics['spatial'] = json.load(f)

    return metrics


def compare_waveforms(baseline_wav: Path, current_wav: Path) -> Dict:
    """Compare two waveform files and return similarity metrics."""
    # Load both files
    sr_baseline, audio_baseline = wavfile.read(baseline_wav)
    sr_current, audio_current = wavfile.read(current_wav)

    if sr_baseline != sr_current:
        return {'error': 'Sample rate mismatch'}

    # Convert to float and normalize
    audio_baseline = audio_baseline.astype(np.float32) / 32768.0
    audio_current = audio_current.astype(np.float32) / 32768.0

    # Handle stereo
    if audio_baseline.ndim > 1:
        audio_baseline = np.mean(audio_baseline, axis=1)
    if audio_current.ndim > 1:
        audio_current = np.mean(audio_current, axis=1)

    # Match lengths
    min_len = min(len(audio_baseline), len(audio_current))
    audio_baseline = audio_baseline[:min_len]
    audio_current = audio_current[:min_len]

    # Calculate difference metrics
    diff = audio_baseline - audio_current
    rms_diff = np.sqrt(np.mean(diff ** 2))

    # Calculate correlation
    correlation = np.corrcoef(audio_baseline, audio_current)[0, 1]

    # Spectral difference
    f_baseline, psd_baseline = signal.welch(audio_baseline, sr_baseline, nperseg=2048)
    f_current, psd_current = signal.welch(audio_current, sr_current, nperseg=2048)

    psd_diff = np.sqrt(np.mean((10 * np.log10(psd_baseline + 1e-10) -
                                 10 * np.log10(psd_current + 1e-10)) ** 2))

    return {
        'rms_difference': float(rms_diff),
        'correlation': float(correlation),
        'spectral_difference_db': float(psd_diff)
    }


def compare_rt60(baseline: Dict, current: Dict, threshold: float) -> Tuple[bool, str, float]:
    """Compare RT60 values between baseline and current."""
    # RT60 can be at top level or under 'broadband' key
    baseline_rt60 = baseline.get('rt60_seconds') or baseline.get('broadband', {}).get('rt60_seconds')
    current_rt60 = current.get('rt60_seconds') or current.get('broadband', {}).get('rt60_seconds')

    if baseline_rt60 is None or current_rt60 is None:
        return False, "Missing RT60 data", 0.0

    diff_pct = abs(baseline_rt60 - current_rt60) / baseline_rt60 * 100

    if diff_pct > threshold * 100:
        return False, f"RT60 changed by {diff_pct:.1f}% ({baseline_rt60:.3f}s → {current_rt60:.3f}s)", diff_pct
    else:
        return True, f"RT60 within threshold ({diff_pct:.1f}% change)", diff_pct


def compare_frequency_response(baseline: Dict, current: Dict, threshold: float) -> Tuple[bool, str, float]:
    """Compare frequency response between baseline and current."""
    baseline_flatness = baseline.get('broadband', {}).get('flatness_db')
    current_flatness = current.get('broadband', {}).get('flatness_db')

    if baseline_flatness is None:
        baseline_flatness = baseline.get('overall', {}).get('flatness_std_db')
    if current_flatness is None:
        current_flatness = current.get('overall', {}).get('flatness_std_db')

    if baseline_flatness is None or current_flatness is None:
        return False, "Missing frequency response data", 0.0

    diff = abs(baseline_flatness - current_flatness)

    if diff > threshold * 10:  # Scale threshold for dB
        return False, f"Flatness changed by {diff:.2f}dB (±{baseline_flatness:.2f}dB → ±{current_flatness:.2f}dB)", diff
    else:
        return True, f"Frequency response within threshold ({diff:.2f}dB change)", diff


def compare_spatial_metrics(
    baseline: Dict,
    current: Dict,
    itd_ms_threshold: float,
    ild_db_threshold: float,
    iacc_threshold: float
) -> Tuple[bool, List[str], Dict[str, float]]:
    """Compare spatial metrics (ITD/ILD/IACC) between baseline and current."""
    issues = []
    metrics = {}

    baseline_bb = baseline.get('broadband', {})
    current_bb = current.get('broadband', {})

    baseline_itd = baseline_bb.get('itd_seconds')
    current_itd = current_bb.get('itd_seconds')
    if baseline_itd is not None and current_itd is not None:
        itd_delta_ms = abs(baseline_itd - current_itd) * 1000.0
        metrics['itd_delta_ms'] = itd_delta_ms
        if itd_delta_ms > itd_ms_threshold:
            issues.append(
                f"ITD changed by {itd_delta_ms:.3f}ms ({baseline_itd * 1000.0:.3f}ms → {current_itd * 1000.0:.3f}ms)"
            )

    baseline_ild = baseline_bb.get('ild_db')
    current_ild = current_bb.get('ild_db')
    if baseline_ild is not None and current_ild is not None:
        ild_delta_db = abs(baseline_ild - current_ild)
        metrics['ild_db_delta'] = ild_delta_db
        if ild_delta_db > ild_db_threshold:
            issues.append(
                f"ILD changed by {ild_delta_db:.2f}dB ({baseline_ild:.2f}dB → {current_ild:.2f}dB)"
            )

    baseline_iacc = baseline_bb.get('iacc')
    current_iacc = current_bb.get('iacc')
    if baseline_iacc is not None and current_iacc is not None:
        iacc_delta = abs(baseline_iacc - current_iacc)
        metrics['iacc_delta'] = iacc_delta
        if iacc_delta > iacc_threshold:
            issues.append(
                f"IACC changed by {iacc_delta:.3f} ({baseline_iacc:.3f} → {current_iacc:.3f})"
            )

    return len(issues) == 0, issues, metrics


def compare_preset(
    preset_idx: int,
    baseline_dir: Path,
    current_dir: Path,
    threshold: float,
    itd_ms_threshold: float,
    ild_db_threshold: float,
    iacc_threshold: float
) -> Dict:
    """Compare a single preset between baseline and current."""
    baseline_preset = baseline_dir / f"preset_{preset_idx:02d}"
    current_preset = current_dir / f"preset_{preset_idx:02d}"

    result = {
        'preset_index': preset_idx,
        'pass': True,
        'issues': [],
        'metrics': {}
    }

    # Check if both presets exist
    if not baseline_preset.exists():
        result['pass'] = False
        result['issues'].append("Baseline preset missing")
        return result

    if not current_preset.exists():
        result['pass'] = False
        result['issues'].append("Current preset missing")
        return result

    # Load metrics
    baseline_metrics = load_metrics(baseline_preset)
    current_metrics = load_metrics(current_preset)

    # Compare RT60
    if baseline_metrics.get('rt60') and current_metrics.get('rt60'):
        pass_rt60, msg, diff = compare_rt60(
            baseline_metrics['rt60'],
            current_metrics['rt60'],
            threshold
        )
        result['metrics']['rt60_diff_pct'] = diff
        if not pass_rt60:
            result['pass'] = False
            result['issues'].append(f"RT60: {msg}")

    # Compare frequency response
    if baseline_metrics.get('freq_response') and current_metrics.get('freq_response'):
        pass_freq, msg, diff = compare_frequency_response(
            baseline_metrics['freq_response'],
            current_metrics['freq_response'],
            threshold
        )
        result['metrics']['freq_flatness_diff_db'] = diff
        if not pass_freq:
            result['pass'] = False
            result['issues'].append(f"Frequency: {msg}")

    # Compare spatial metrics
    if baseline_metrics.get('spatial') and current_metrics.get('spatial'):
        pass_spatial, issues, spatial_metrics = compare_spatial_metrics(
            baseline_metrics['spatial'],
            current_metrics['spatial'],
            itd_ms_threshold,
            ild_db_threshold,
            iacc_threshold
        )
        if spatial_metrics:
            result['metrics']['spatial'] = spatial_metrics
        if not pass_spatial:
            result['pass'] = False
            for issue in issues:
                result['issues'].append(f"Spatial: {issue}")

    # Compare waveforms
    baseline_wav = baseline_preset / "wet.wav"
    current_wav = current_preset / "wet.wav"

    if baseline_wav.exists() and current_wav.exists():
        waveform_metrics = compare_waveforms(baseline_wav, current_wav)

        if 'error' not in waveform_metrics:
            result['metrics']['waveform'] = waveform_metrics

            # Check correlation
            if waveform_metrics['correlation'] < 0.95:
                result['pass'] = False
                result['issues'].append(
                    f"Waveform correlation low: {waveform_metrics['correlation']:.4f}"
                )

            # Check RMS difference
            if waveform_metrics['rms_difference'] > threshold:
                result['pass'] = False
                result['issues'].append(
                    f"Waveform RMS difference: {waveform_metrics['rms_difference']:.6f}"
                )

    return result


def main():
    parser = argparse.ArgumentParser(
        description='Compare Monument Reverb presets against baseline',
        formatter_class=argparse.RawDescriptionHelpFormatter
    )

    parser.add_argument('baseline_dir', type=Path,
                       help='Baseline preset directory')
    parser.add_argument('current_dir', type=Path,
                       help='Current preset directory to compare')
    parser.add_argument('--threshold', '-t', type=float, default=0.05,
                       help='Difference threshold (0.05 = 5%% change, default: 0.05)')
    parser.add_argument('--spatial-itd-ms', type=float, default=DEFAULT_SPATIAL_ITD_MS,
                       help=f'Spatial ITD threshold in ms (default: {DEFAULT_SPATIAL_ITD_MS})')
    parser.add_argument('--spatial-ild-db', type=float, default=DEFAULT_SPATIAL_ILD_DB,
                       help=f'Spatial ILD threshold in dB (default: {DEFAULT_SPATIAL_ILD_DB})')
    parser.add_argument('--spatial-iacc', type=float, default=DEFAULT_SPATIAL_IACC_DELTA,
                       help=f'Spatial IACC threshold (default: {DEFAULT_SPATIAL_IACC_DELTA})')
    parser.add_argument('--output', '-o', type=Path,
                       help='Output JSON report file (optional)')
    parser.add_argument('--preset', '-p', type=int, nargs='+',
                       help='Compare specific presets only (e.g., --preset 0 7 12)')

    args = parser.parse_args()

    if not args.baseline_dir.exists():
        print(f"Error: Baseline directory not found: {args.baseline_dir}")
        return 1

    if not args.current_dir.exists():
        print(f"Error: Current directory not found: {args.current_dir}")
        return 1

    print("\n" + "=" * 70)
    print("Monument Reverb - Baseline Comparison Tool")
    print("=" * 70 + "\n")
    print(f"Baseline: {args.baseline_dir}")
    print(f"Current:  {args.current_dir}")
    print(f"Threshold: {args.threshold * 100:.1f}%\n")
    print(
        f"Spatial thresholds: ITD {args.spatial_itd_ms:.3f}ms, "
        f"ILD {args.spatial_ild_db:.2f}dB, "
        f"IACC {args.spatial_iacc:.3f}\n"
    )

    # Determine which presets to compare
    if args.preset:
        preset_indices = args.preset
    else:
        preset_indices = range(37)

    # Compare all presets
    results = []
    pass_count = 0
    fail_count = 0

    for i in preset_indices:
        result = compare_preset(
            i,
            args.baseline_dir,
            args.current_dir,
            args.threshold,
            args.spatial_itd_ms,
            args.spatial_ild_db,
            args.spatial_iacc
        )
        results.append(result)

        if result['pass']:
            pass_count += 1
            status = "✓"
            color = "\033[32m"  # Green
        else:
            fail_count += 1
            status = "✗"
            color = "\033[31m"  # Red

        reset = "\033[0m"
        print(f"{color}[{status}] Preset {i:02d}{reset}", end="")

        if not result['pass']:
            print(f" - {', '.join(result['issues'])}")
        else:
            # Show minor details for passing presets
            metrics_summary = []
            if 'rt60_diff_pct' in result['metrics']:
                metrics_summary.append(f"RT60: {result['metrics']['rt60_diff_pct']:.1f}%")
            if 'freq_flatness_diff_db' in result['metrics']:
                metrics_summary.append(f"Flatness: {result['metrics']['freq_flatness_diff_db']:.2f}dB")
            if 'spatial' in result['metrics']:
                spatial = result['metrics']['spatial']
                itd_delta = spatial.get('itd_delta_ms', 0.0)
                ild_delta = spatial.get('ild_db_delta', 0.0)
                iacc_delta = spatial.get('iacc_delta', 0.0)
                metrics_summary.append(
                    f"Spatial: ITD {itd_delta:.2f}ms, ILD {ild_delta:.2f}dB, IACC {iacc_delta:.3f}"
                )
            if metrics_summary:
                print(f" ({', '.join(metrics_summary)})")
            else:
                print()

    # Summary
    print("\n" + "=" * 70)
    print("Summary")
    print("=" * 70)
    print(f"Passed: {pass_count}/{len(results)}")
    print(f"Failed: {fail_count}/{len(results)}")
    print()

    # Save report if requested
    if args.output:
        report = {
            'baseline_dir': str(args.baseline_dir),
            'current_dir': str(args.current_dir),
            'threshold': args.threshold,
            'summary': {
                'total': len(results),
                'passed': pass_count,
                'failed': fail_count
            },
            'results': results
        }

        with open(args.output, 'w') as f:
            json.dump(report, f, indent=2)

        print(f"✓ Saved report to: {args.output}\n")

    # Exit code
    if fail_count > 0:
        print("\033[31m⚠ REGRESSION DETECTED - Some presets failed comparison\033[0m\n")
        return 1
    else:
        print("\033[32m✓ All presets passed - No regressions detected\033[0m\n")
        return 0


if __name__ == '__main__':
    sys.exit(main())
