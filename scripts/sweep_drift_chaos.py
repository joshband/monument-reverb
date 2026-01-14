#!/usr/bin/env python3
"""
Sweep Drift + Chaos across presets, capture audio, and analyze metrics.
"""

import argparse
import csv
import itertools
import json
import math
import os
import subprocess
import sys
import tempfile
from datetime import datetime, timezone
from pathlib import Path
from typing import List, Optional, Tuple

DEFAULT_PLUGIN_PATH = Path.home() / "Library/Audio/Plug-Ins/VST3/Monument.vst3"


def resolve_project_root() -> Path:
    return Path(__file__).resolve().parent.parent


def resolve_build_dir(project_root: Path, build_dir: Optional[str]) -> Path:
    if build_dir:
        path = Path(build_dir)
        if not path.is_absolute():
            path = project_root / path
        return path
    for candidate in (project_root / "build", project_root / "build-ninja"):
        if candidate.exists():
            return candidate
    return project_root / "build"


def find_analyzer(build_dir: Path, config: str) -> Optional[Path]:
    candidates = [
        build_dir / "monument_plugin_analyzer_artefacts" / config / "monument_plugin_analyzer",
        build_dir / "monument_plugin_analyzer_artefacts" / "Debug" / "monument_plugin_analyzer",
        build_dir / "monument_plugin_analyzer_artefacts" / "Release" / "monument_plugin_analyzer",
        build_dir / "monument_plugin_analyzer_artefacts" / "RelWithDebInfo" / "monument_plugin_analyzer",
        build_dir / "monument_plugin_analyzer_artefacts" / "monument_plugin_analyzer",
    ]
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    return None


def resolve_python(project_root: Path) -> str:
    env_python = os.environ.get("PYTHON_BIN")
    if env_python:
        return env_python

    venv_path = os.environ.get("VENV_PATH")
    if venv_path:
        candidate = Path(venv_path) / "bin/python"
        if candidate.exists():
            return str(candidate)

    candidate = project_root / ".venv" / "bin" / "python"
    if candidate.exists():
        return str(candidate)

    return "python3"


def utc_timestamp() -> str:
    return datetime.now(timezone.utc).replace(microsecond=0).isoformat().replace("+00:00", "Z")


def parse_index_list(text: str) -> List[int]:
    indices = set()
    for token in text.split(","):
        token = token.strip()
        if not token:
            continue
        if "-" in token:
            start, end = token.split("-", 1)
            indices.update(range(int(start), int(end) + 1))
        else:
            indices.add(int(token))
    return sorted(indices)


def parse_float_list(text: str) -> List[float]:
    values = []
    for token in text.split(","):
        token = token.strip()
        if not token:
            continue
        values.append(float(token))
    return values


def format_value(value: float) -> str:
    text = f"{value:.4f}".rstrip("0").rstrip(".")
    return text if text else "0"

def format_optional(value: Optional[float], precision: int = 3) -> str:
    if value is None:
        return "n/a"
    return f"{value:.{precision}f}"


def clamp_unit(value: float, label: str) -> float:
    if value < 0.0 or value > 1.0:
        clamped = max(0.0, min(1.0, value))
        print(f"Warning: {label}={value} out of range, clamping to {clamped}")
        return clamped
    return value


def run_command(cmd: List[str], log_path: Path) -> int:
    log_path.parent.mkdir(parents=True, exist_ok=True)
    with log_path.open("w", encoding="utf-8") as log_file:
        result = subprocess.run(cmd, stdout=log_file, stderr=subprocess.STDOUT)
    return result.returncode


def load_json(path: Path) -> Optional[dict]:
    if not path.exists():
        return None
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def detect_runaway(rt60_data: Optional[dict], duration_seconds: float) -> Tuple[bool, str]:
    if not rt60_data:
        return False, ""

    notes = str(rt60_data.get("analysis_notes", ""))
    rt60 = float(rt60_data.get("broadband", {}).get("rt60_seconds", 0.0) or 0.0)
    if notes and "decay" in notes.lower():
        return True, notes

    if rt60 > 0.0 and rt60 >= duration_seconds * 0.9:
        return True, f"rt60 >= {duration_seconds * 0.9:.2f}s"

    return False, ""


def compute_basic_stats(values: List[float]) -> Optional[dict]:
    if not values:
        return None

    values_sorted = sorted(values)
    count = len(values_sorted)
    total = sum(values_sorted)
    mean = total / count

    if count % 2 == 1:
        median = values_sorted[count // 2]
    else:
        median = 0.5 * (values_sorted[count // 2 - 1] + values_sorted[count // 2])

    def percentile(p: float) -> float:
        if count == 1:
            return values_sorted[0]
        k = (count - 1) * p
        f = math.floor(k)
        c = math.ceil(k)
        if f == c:
            return values_sorted[int(k)]
        return values_sorted[f] * (c - k) + values_sorted[c] * (k - f)

    return {
        "count": count,
        "min": values_sorted[0],
        "max": values_sorted[-1],
        "mean": mean,
        "median": median,
        "p95": percentile(0.95),
    }


def summarize_by_preset(results: List[dict]) -> List[dict]:
    summary = {}
    for run in results:
        preset = run["preset"]
        entry = summary.setdefault(
            preset,
            {
                "preset": preset,
                "runs": 0,
                "rt60_values": [],
                "flatness_values": [],
                "runaways": 0,
                "stability_failures": 0,
            },
        )
        entry["runs"] += 1
        if run.get("rt60_seconds") is not None:
            entry["rt60_values"].append(run["rt60_seconds"])
        if run.get("flatness_db") is not None:
            entry["flatness_values"].append(run["flatness_db"])
        if run.get("runaway"):
            entry["runaways"] += 1
        if run.get("stability_ok") is False:
            entry["stability_failures"] += 1

    preset_rows = []
    for preset in sorted(summary.keys()):
        entry = summary[preset]
        rt60_stats = compute_basic_stats(entry["rt60_values"]) or {}
        flatness_stats = compute_basic_stats(entry["flatness_values"]) or {}
        preset_rows.append(
            {
                "preset": preset,
                "runs": entry["runs"],
                "rt60_mean": rt60_stats.get("mean"),
                "rt60_max": rt60_stats.get("max"),
                "flatness_mean": flatness_stats.get("mean"),
                "runaways": entry["runaways"],
                "stability_failures": entry["stability_failures"],
            }
        )
    return preset_rows


def configure_matplotlib_cache() -> None:
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


def build_metric_grid(results: List[dict], drift_values: List[float], chaos_values: List[float], key: str) -> List[List[float]]:
    grid = [[float("nan") for _ in drift_values] for _ in chaos_values]
    for chaos_index, chaos in enumerate(chaos_values):
        for drift_index, drift in enumerate(drift_values):
            values = [
                run[key]
                for run in results
                if run.get("drift") == drift and run.get("chaosIntensity") == chaos and run.get(key) is not None
            ]
            if values:
                grid[chaos_index][drift_index] = sum(values) / len(values)
    return grid


def build_count_grid(results: List[dict], drift_values: List[float], chaos_values: List[float], predicate_key: str) -> List[List[int]]:
    grid = [[0 for _ in drift_values] for _ in chaos_values]
    for chaos_index, chaos in enumerate(chaos_values):
        for drift_index, drift in enumerate(drift_values):
            grid[chaos_index][drift_index] = sum(
                1
                for run in results
                if run.get("drift") == drift and run.get("chaosIntensity") == chaos and run.get(predicate_key)
            )
    return grid


def generate_summary_plots(
    results: List[dict],
    output_base: Path,
    drift_values: List[float],
    chaos_values: List[float],
    notes: List[str],
) -> None:
    try:
        configure_matplotlib_cache()
        import matplotlib.pyplot as plt  # pylint: disable=import-error
        import numpy as np  # pylint: disable=import-error
    except Exception as exc:
        notes.append(f"Summary plots skipped: {exc}")
        return

    if not results:
        notes.append("Summary plots skipped: no results")
        return

    drift_labels = [format_value(value) for value in drift_values]
    chaos_labels = [format_value(value) for value in chaos_values]

    def render_heatmap(grid: List[List[float]], title: str, output_name: str, colorbar_label: str) -> None:
        data = np.array(grid, dtype=float)
        fig, ax = plt.subplots(figsize=(6.5, 4.5))
        im = ax.imshow(data, origin="lower", aspect="auto")
        ax.set_xticks(range(len(drift_labels)))
        ax.set_xticklabels(drift_labels)
        ax.set_yticks(range(len(chaos_labels)))
        ax.set_yticklabels(chaos_labels)
        ax.set_xlabel("Drift")
        ax.set_ylabel("Chaos")
        ax.set_title(title)
        cbar = fig.colorbar(im, ax=ax)
        cbar.ax.set_ylabel(colorbar_label)
        plt.tight_layout()
        output_path = output_base / output_name
        fig.savefig(output_path, dpi=150)
        plt.close(fig)

    rt60_grid = build_metric_grid(results, drift_values, chaos_values, "rt60_seconds")
    render_heatmap(rt60_grid, "RT60 Mean by Drift/Chaos", "rt60_heatmap.png", "RT60 (s)")

    flatness_grid = build_metric_grid(results, drift_values, chaos_values, "flatness_db")
    render_heatmap(flatness_grid, "Flatness Mean by Drift/Chaos", "flatness_heatmap.png", "Flatness (dB)")

    runaway_grid = build_count_grid(results, drift_values, chaos_values, "runaway")
    render_heatmap(runaway_grid, "Runaway Count by Drift/Chaos", "runaway_heatmap.png", "Runaway count")

    stability_grid = build_count_grid(results, drift_values, chaos_values, "stability_ok")
    render_heatmap(stability_grid, "Stability Pass Count by Drift/Chaos", "stability_heatmap.png", "Pass count")


def write_report(
    output_base: Path,
    manifest: dict,
    summary: dict,
    results: List[dict],
    preset_rows: List[dict],
    notes: List[str],
) -> Path:
    report_lines = []
    report_lines.append("# Drift/Chaos Sweep Report")
    report_lines.append("")
    report_lines.append(f"Generated: {summary.get('timestamp', 'n/a')}")
    report_lines.append("")
    report_lines.append("## Overview")
    report_lines.append(f"- Runs: {summary.get('runs_total', 0)}")
    report_lines.append(f"- Failures: {summary.get('failures', 0)}")
    report_lines.append(f"- Runaways: {summary.get('runaway_count', 0)}")
    report_lines.append(f"- Stability failures: {summary.get('stability_failures', 0)}")
    report_lines.append(f"- Duration: {manifest.get('duration_seconds', 'n/a')} seconds")
    report_lines.append(f"- Presets: {manifest.get('presets', [])}")
    report_lines.append(f"- Drift values: {manifest.get('drift_values', [])}")
    report_lines.append(f"- Chaos values: {manifest.get('chaos_values', [])}")
    report_lines.append("")

    report_lines.append("## Aggregate Metrics")
    report_lines.append("| Metric | Min | Mean | Median | P95 | Max |")
    report_lines.append("| --- | --- | --- | --- | --- | --- |")

    rt60_stats = summary.get("rt60_stats") or {}
    flatness_stats = summary.get("flatness_stats") or {}
    report_lines.append(
        "| RT60 (s) | "
        f"{format_optional(rt60_stats.get('min'))} | "
        f"{format_optional(rt60_stats.get('mean'))} | "
        f"{format_optional(rt60_stats.get('median'))} | "
        f"{format_optional(rt60_stats.get('p95'))} | "
        f"{format_optional(rt60_stats.get('max'))} |"
    )
    report_lines.append(
        "| Flatness (dB) | "
        f"{format_optional(flatness_stats.get('min'))} | "
        f"{format_optional(flatness_stats.get('mean'))} | "
        f"{format_optional(flatness_stats.get('median'))} | "
        f"{format_optional(flatness_stats.get('p95'))} | "
        f"{format_optional(flatness_stats.get('max'))} |"
    )
    report_lines.append("")

    if summary.get("max_rt60"):
        max_rt60 = summary["max_rt60"]
        report_lines.append("## Extremes")
        report_lines.append(
            f"- Max RT60: {format_optional(max_rt60.get('rt60_seconds'))} s "
            f"(preset {max_rt60.get('preset')}, drift {max_rt60.get('drift')}, chaos {max_rt60.get('chaosIntensity')})"
        )
        report_lines.append("")

    if preset_rows:
        report_lines.append("## Per-Preset Summary")
        report_lines.append("| Preset | Runs | RT60 mean (s) | RT60 max (s) | Flatness mean (dB) | Runaways | Stability failures |")
        report_lines.append("| --- | --- | --- | --- | --- | --- | --- |")
        for row in preset_rows:
            report_lines.append(
                f"| {row['preset']} | {row['runs']} | "
                f"{format_optional(row.get('rt60_mean'))} | "
                f"{format_optional(row.get('rt60_max'))} | "
                f"{format_optional(row.get('flatness_mean'))} | "
                f"{row.get('runaways', 0)} | {row.get('stability_failures', 0)} |"
            )
        report_lines.append("")

    runaway_runs = [run for run in results if run.get("runaway")]
    if runaway_runs:
        report_lines.append("## Runaway Details")
        for run in runaway_runs:
            report_lines.append(
                f"- preset {run['preset']} drift {run['drift']} chaos {run['chaosIntensity']}: {run.get('runaway_reason', '')}"
            )
        report_lines.append("")

    if notes:
        report_lines.append("## Notes")
        for note in notes:
            report_lines.append(f"- {note}")
        report_lines.append("")

    report_lines.append("## Files")
    report_lines.append(f"- sweep_summary.json")
    report_lines.append(f"- sweep_summary.csv")
    report_lines.append(f"- sweep_manifest.json")
    report_lines.append(f"- sweep_report.md")

    report_path = output_base / "sweep_report.md"
    report_path.write_text("\n".join(report_lines) + "\n", encoding="utf-8")
    return report_path


def main() -> int:
    project_root = resolve_project_root()

    parser = argparse.ArgumentParser(
        description="Sweep Drift/Chaos across presets and capture metrics"
    )
    parser.add_argument("--plugin", type=Path, default=DEFAULT_PLUGIN_PATH,
                        help="Path to Monument VST3/AU plugin")
    parser.add_argument("--build-dir", type=str, default=None,
                        help="Build directory (default: build or build-ninja)")
    parser.add_argument("--config", type=str, default="Debug",
                        help="Build config for analyzer lookup (default: Debug)")
    parser.add_argument("--output", type=Path, default=project_root / "test-results" / "drift-chaos-sweep",
                        help="Output directory for sweep data")
    parser.add_argument("--presets", type=str, default="0-7",
                        help="Preset indices (e.g. 0-7, 0,2,4, 0-36)")
    parser.add_argument("--drift", type=str, default="0,0.5,1.0",
                        help="Drift values (comma-separated, 0-1)")
    parser.add_argument("--chaos", type=str, default="0,0.5,1.0",
                        help="Chaos values (comma-separated, 0-1)")
    parser.add_argument("--duration", type=float, default=30.0,
                        help="Capture duration in seconds (default: 30)")
    parser.add_argument("--samplerate", type=float, default=48000.0,
                        help="Sample rate in Hz (default: 48000)")
    parser.add_argument("--blocksize", type=int, default=512,
                        help="Block size in samples (default: 512)")
    parser.add_argument("--channels", type=int, default=2,
                        help="Number of channels (default: 2)")
    parser.add_argument("--plots", action="store_true",
                        help="Generate RT60/frequency plots")
    parser.add_argument("--force", action="store_true",
                        help="Re-capture and re-analyze even if files exist")
    parser.add_argument("--no-analysis", action="store_true",
                        help="Skip RT60/frequency analysis")
    parser.add_argument("--strict", action="store_true",
                        help="Exit non-zero on any capture/analysis failure")

    args = parser.parse_args()

    build_dir = resolve_build_dir(project_root, args.build_dir)
    analyzer_path = find_analyzer(build_dir, args.config)
    if analyzer_path is None:
        print("Error: monument_plugin_analyzer not found. Build it first.")
        return 2

    if not args.plugin.exists():
        print(f"Error: plugin not found at {args.plugin}")
        return 2

    rt60_script = project_root / "tools" / "plugin-analyzer" / "python" / "rt60_analysis_robust.py"
    freq_script = project_root / "tools" / "plugin-analyzer" / "python" / "frequency_response.py"
    stability_script = project_root / "tools" / "check_audio_stability.py"

    python_bin = resolve_python(project_root)

    presets = parse_index_list(args.presets)
    drift_values = [clamp_unit(v, "drift") for v in parse_float_list(args.drift)]
    chaos_values = [clamp_unit(v, "chaos") for v in parse_float_list(args.chaos)]

    if not presets:
        print("Error: no presets specified")
        return 2

    output_base = args.output
    output_base.mkdir(parents=True, exist_ok=True)

    manifest = {
        "timestamp": utc_timestamp(),
        "plugin_path": str(args.plugin),
        "analyzer_path": str(analyzer_path),
        "presets": presets,
        "drift_values": drift_values,
        "chaos_values": chaos_values,
        "duration_seconds": args.duration,
        "sample_rate": args.samplerate,
        "block_size": args.blocksize,
        "channels": args.channels,
        "analysis": not args.no_analysis,
        "plots": args.plots,
    }

    (output_base / "sweep_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    results = []
    failures = 0

    for preset, drift, chaos in itertools.product(presets, drift_values, chaos_values):
        drift_label = format_value(drift)
        chaos_label = format_value(chaos)
        run_dir = output_base / f"preset_{preset:02d}" / f"drift_{drift_label}_chaos_{chaos_label}"
        run_dir.mkdir(parents=True, exist_ok=True)

        dry_wav = run_dir / "dry.wav"
        wet_wav = run_dir / "wet.wav"
        rt60_json = run_dir / "rt60_metrics.json"
        freq_json = run_dir / "freq_metrics.json"

        capture_needed = args.force or not (dry_wav.exists() and wet_wav.exists())
        if capture_needed:
            print(f"Capture preset {preset} drift={drift_label} chaos={chaos_label}")
            cmd = [
                str(analyzer_path),
                "--plugin", str(args.plugin),
                "--preset", str(preset),
                "--duration", str(args.duration),
                "--samplerate", str(args.samplerate),
                "--channels", str(args.channels),
                "--blocksize", str(args.blocksize),
                "--output", str(run_dir),
                "--param", f"drift={drift}",
                "--param", f"chaosIntensity={chaos}",
            ]
            capture_code = run_command(cmd, run_dir / "capture.log")
            if capture_code != 0:
                print(f"  Capture failed (exit {capture_code})")
                failures += 1
                if args.strict:
                    return 1
        else:
            print(f"Reuse capture preset {preset} drift={drift_label} chaos={chaos_label}")

        metadata = {
            "version": "1.0.0",
            "timestamp": utc_timestamp(),
            "plugin_path": str(args.plugin),
            "preset_index": preset,
            "duration_seconds": args.duration,
            "sample_rate": args.samplerate,
            "num_channels": args.channels,
            "block_size": args.blocksize,
            "test_type": "impulse",
            "parameters": {
                "drift": drift,
                "chaosIntensity": chaos,
            },
            "output_files": {
                "dry": "dry.wav",
                "wet": "wet.wav",
            },
        }
        (run_dir / "metadata.json").write_text(json.dumps(metadata, indent=2), encoding="utf-8")

        rt60_data = None
        freq_data = None
        stability_ok = None

        if not args.no_analysis and wet_wav.exists():
            rt60_needed = args.force or not rt60_json.exists()
            if rt60_needed:
                rt60_cmd = [python_bin, str(rt60_script), str(wet_wav), "--output", str(rt60_json)]
                if not args.plots:
                    rt60_cmd.append("--no-plot")
                rt60_code = run_command(rt60_cmd, run_dir / "rt60_analysis.log")
                if rt60_code != 0:
                    print(f"  RT60 analysis failed (exit {rt60_code})")
                    failures += 1
                    if args.strict:
                        return 1

            freq_needed = args.force or not freq_json.exists()
            if freq_needed:
                freq_cmd = [python_bin, str(freq_script), str(wet_wav), "--impulse", "--output", str(freq_json)]
                if not args.plots:
                    freq_cmd.append("--no-plot")
                freq_code = run_command(freq_cmd, run_dir / "freq_analysis.log")
                if freq_code != 0:
                    print(f"  Frequency analysis failed (exit {freq_code})")
                    failures += 1
                    if args.strict:
                        return 1

            stability_cmd = [python_bin, str(stability_script), str(wet_wav)]
            stability_code = run_command(stability_cmd, run_dir / "stability.log")
            stability_ok = (stability_code == 0)

            rt60_data = load_json(rt60_json)
            freq_data = load_json(freq_json)

        runaway, runaway_reason = detect_runaway(rt60_data, args.duration)

        rt60_seconds = None
        rt60_confidence = None
        dynamic_range_db = None
        rms = None
        peak = None
        if rt60_data:
            rt60_seconds = rt60_data.get("broadband", {}).get("rt60_seconds")
            rt60_confidence = rt60_data.get("broadband", {}).get("confidence")
            metadata_rt60 = rt60_data.get("_metadata", {})
            dynamic_range_db = metadata_rt60.get("dynamic_range_db")
            rms = metadata_rt60.get("rms")
            peak = metadata_rt60.get("peak")

        flatness_db = None
        mean_gain_db = None
        if freq_data:
            flatness_db = freq_data.get("broadband", {}).get("flatness_db")
            mean_gain_db = freq_data.get("broadband", {}).get("mean_gain_db")

        results.append({
            "preset": preset,
            "drift": drift,
            "chaosIntensity": chaos,
            "duration_seconds": args.duration,
            "rt60_seconds": rt60_seconds,
            "rt60_confidence": rt60_confidence,
            "dynamic_range_db": dynamic_range_db,
            "rms": rms,
            "peak": peak,
            "flatness_db": flatness_db,
            "mean_gain_db": mean_gain_db,
            "stability_ok": stability_ok,
            "runaway": runaway,
            "runaway_reason": runaway_reason,
            "output_dir": str(run_dir.relative_to(output_base)),
        })

    summary = {
        "timestamp": utc_timestamp(),
        "runs_total": len(results),
        "failures": failures,
        "runaway_count": sum(1 for item in results if item["runaway"]),
        "stability_failures": sum(1 for item in results if item["stability_ok"] is False),
    }

    rt60_values = [item["rt60_seconds"] for item in results if item["rt60_seconds"]]
    flatness_values = [item["flatness_db"] for item in results if item["flatness_db"]]
    if rt60_values:
        max_rt60 = max(rt60_values)
        max_item = next(item for item in results if item["rt60_seconds"] == max_rt60)
        summary["max_rt60"] = {
            "rt60_seconds": max_rt60,
            "preset": max_item["preset"],
            "drift": max_item["drift"],
            "chaosIntensity": max_item["chaosIntensity"],
        }
    summary["rt60_stats"] = compute_basic_stats(rt60_values)
    summary["flatness_stats"] = compute_basic_stats(flatness_values)

    summary_path = output_base / "sweep_summary.json"
    summary_path.write_text(json.dumps({"summary": summary, "runs": results}, indent=2), encoding="utf-8")

    csv_path = output_base / "sweep_summary.csv"
    with csv_path.open("w", newline="", encoding="utf-8") as csv_file:
        fieldnames = list(results[0].keys()) if results else []
        writer = csv.DictWriter(csv_file, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(results)

    notes = []
    preset_rows = summarize_by_preset(results)
    if args.plots:
        generate_summary_plots(results, output_base, drift_values, chaos_values, notes)
    report_path = write_report(output_base, manifest, summary, results, preset_rows, notes)

    print("\nSweep complete")
    print(f"Runs: {len(results)}")
    print(f"Failures: {failures}")
    print(f"Runaways: {summary.get('runaway_count', 0)}")
    print(f"Summary: {summary_path}")
    print(f"Report: {report_path}")

    if failures and args.strict:
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
