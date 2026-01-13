#!/usr/bin/env python3
"""
Monument UI visual regression testing.

Compares screenshots in a baseline directory against a current directory,
generates diff images, and writes an HTML report + JSON summary.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Tuple

import numpy as np
from PIL import Image, ImageChops


DEFAULT_THRESHOLD = 0.02
DEFAULT_BACKGROUND_DIFF = 100.0
DEFAULT_DIFF_AMPLIFY = 10


def load_image(path: Path) -> Image.Image:
    return Image.open(path).convert("RGB")


def compute_background_brightness(arr: np.ndarray, sample_size: int = 20) -> float:
    height, width = arr.shape[:2]
    size = min(sample_size, height, width)
    corners = [
        arr[:size, :size],
        arr[:size, width - size : width],
        arr[height - size : height, :size],
        arr[height - size : height, width - size : width],
    ]
    samples = np.concatenate([c.reshape(-1, 3) for c in corners], axis=0)
    r = samples[:, 0].astype(np.float32)
    g = samples[:, 1].astype(np.float32)
    b = samples[:, 2].astype(np.float32)
    return float(0.2126 * r.mean() + 0.7152 * g.mean() + 0.0722 * b.mean())


def compute_metrics(
    baseline: Image.Image,
    current: Image.Image,
    background_threshold: float,
) -> Dict[str, float]:
    if baseline.size != current.size:
        raise ValueError("size_mismatch")

    base_arr = np.asarray(baseline).astype(np.int16)
    curr_arr = np.asarray(current).astype(np.int16)
    diff_arr = np.abs(base_arr - curr_arr)

    diff_score = float(diff_arr.mean() / 255.0)
    mean_diff = float(diff_arr.mean())
    max_diff = float(diff_arr.max())

    changed_pixels = int(np.count_nonzero(np.any(diff_arr > 0, axis=2)))
    total_pixels = int(diff_arr.shape[0] * diff_arr.shape[1])

    base_bg = compute_background_brightness(base_arr)
    curr_bg = compute_background_brightness(curr_arr)
    background_diff = float(abs(base_bg - curr_bg))
    background_pass = background_diff <= background_threshold

    return {
        "diff_score": diff_score,
        "mean_diff": mean_diff,
        "max_diff": max_diff,
        "changed_pixels": changed_pixels,
        "total_pixels": total_pixels,
        "background_diff": background_diff,
        "background_pass": background_pass,
    }


def amplify_diff(diff_image: Image.Image, factor: int) -> Image.Image:
    return diff_image.point(lambda v: min(255, v * factor))


def write_report(
    output_dir: Path,
    results: Dict,
    threshold: float,
    background_threshold: float,
) -> None:
    report_path = output_dir / "report.html"
    generated_at = results["summary"]["generated_at"]
    total = results["summary"]["total"]
    passed = results["summary"]["passed"]
    failed = results["summary"]["failed"]

    rows = []
    for item in results["tests"]:
        status_class = "pass" if item["status"] == "pass" else "fail"
        diff_score = f"{item.get('diff_score', 0.0):.4f}" if "diff_score" in item else "n/a"
        background_diff = (
            f"{item.get('background_diff', 0.0):.1f}" if "background_diff" in item else "n/a"
        )
        reason = item.get("reason", "")
        rows.append(
            f"""
            <tr class="{status_class}">
              <td>{item["name"]}</td>
              <td>{item["status"].upper()}</td>
              <td>{diff_score}</td>
              <td>{background_diff}</td>
              <td>{reason}</td>
              <td>
                <div class="images">
                  <img src="{item.get('baseline_rel', '')}" alt="baseline">
                  <img src="{item.get('current_rel', '')}" alt="current">
                  <img src="{item.get('diff_rel', '')}" alt="diff">
                  <img src="{item.get('diff_amplified_rel', '')}" alt="diff amplified">
                </div>
              </td>
            </tr>
            """
        )

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Monument UI Visual Regression Report</title>
  <style>
    body {{
      font-family: Arial, sans-serif;
      margin: 20px;
      background: #f6f6f6;
      color: #222;
    }}
    header {{
      background: #111;
      color: #fff;
      padding: 12px 16px;
      border-radius: 8px;
    }}
    table {{
      width: 100%;
      border-collapse: collapse;
      margin-top: 16px;
      background: #fff;
    }}
    th, td {{
      border: 1px solid #ddd;
      padding: 10px;
      font-size: 13px;
      vertical-align: top;
    }}
    th {{
      background: #f0f0f0;
      text-align: left;
    }}
    tr.pass td {{
      background: #f8fff8;
    }}
    tr.fail td {{
      background: #fff5f5;
    }}
    .images {{
      display: grid;
      grid-template-columns: repeat(4, minmax(120px, 1fr));
      gap: 8px;
    }}
    img {{
      max-width: 100%;
      height: auto;
      border: 1px solid #ccc;
    }}
    .summary {{
      margin-top: 10px;
    }}
  </style>
</head>
<body>
  <header>
    <h1>Monument UI Visual Regression Report</h1>
    <div class="summary">
      Generated: {generated_at}<br>
      Threshold: {threshold:.4f} | Background Threshold: {background_threshold:.1f}<br>
      Total: {total} | Passed: {passed} | Failed: {failed}
    </div>
  </header>
  <table>
    <thead>
      <tr>
        <th>Test</th>
        <th>Status</th>
        <th>Diff Score</th>
        <th>Background Diff</th>
        <th>Reason</th>
        <th>Images (Baseline | Current | Diff | Amplified)</th>
      </tr>
    </thead>
    <tbody>
      {''.join(rows)}
    </tbody>
  </table>
</body>
</html>
"""
    report_path.write_text(html, encoding="utf-8")


def main() -> int:
    parser = argparse.ArgumentParser(description="Monument UI visual regression tests.")
    parser.add_argument(
        "--baseline-dir",
        type=Path,
        default=Path("test-results/ui-baseline"),
        help="Directory containing baseline PNG screenshots.",
    )
    parser.add_argument(
        "--current-dir",
        type=Path,
        default=None,
        help="Directory containing current PNG screenshots (defaults to output dir).",
    )
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("test-results/ui-current"),
        help="Output directory for reports/diffs (also used as current dir if not provided).",
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=DEFAULT_THRESHOLD,
        help="Diff score threshold (0.0 - 1.0).",
    )
    parser.add_argument(
        "--background-threshold",
        type=float,
        default=DEFAULT_BACKGROUND_DIFF,
        help="Background brightness delta threshold (0 - 255).",
    )
    parser.add_argument(
        "--diff-amplify",
        type=int,
        default=DEFAULT_DIFF_AMPLIFY,
        help="Amplify diff image by this factor for visibility.",
    )

    args = parser.parse_args()
    baseline_dir = args.baseline_dir
    output_dir = args.output_dir
    current_dir = args.current_dir or output_dir

    if not baseline_dir.exists():
        print(f"ERROR: Baseline directory not found: {baseline_dir}", file=sys.stderr)
        return 1

    baseline_images = sorted(baseline_dir.glob("*.png"))
    if not baseline_images:
        print("ERROR: No baseline PNGs found. Run capture first.", file=sys.stderr)
        return 1

    output_dir.mkdir(parents=True, exist_ok=True)
    diffs_dir = output_dir / "diffs"
    diffs_dir.mkdir(parents=True, exist_ok=True)

    results: List[Dict] = []
    failures = 0

    for baseline_path in baseline_images:
        name = baseline_path.name
        current_path = current_dir / name
        print(f"Testing: {name}")

        if not current_path.exists():
            results.append(
                {
                    "name": name,
                    "status": "fail",
                    "reason": "missing_current",
                }
            )
            failures += 1
            print("  FAIL: missing_current")
            continue

        try:
            baseline_img = load_image(baseline_path)
            current_img = load_image(current_path)
            metrics = compute_metrics(
                baseline_img, current_img, args.background_threshold
            )
        except ValueError:
            results.append(
                {
                    "name": name,
                    "status": "fail",
                    "reason": "size_mismatch",
                }
            )
            failures += 1
            print("  FAIL: size_mismatch")
            continue

        diff_img = ImageChops.difference(baseline_img, current_img)
        diff_path = diffs_dir / f"{baseline_path.stem}_diff.png"
        diff_img.save(diff_path)
        diff_amplified = amplify_diff(diff_img, args.diff_amplify)
        diff_amp_path = diffs_dir / f"{baseline_path.stem}_diff_amplified.png"
        diff_amplified.save(diff_amp_path)

        diff_score = metrics["diff_score"]
        background_pass = metrics["background_pass"]

        status = "pass"
        reason = ""
        if not background_pass:
            status = "fail"
            reason = "background_color_mismatch"
        elif diff_score > args.threshold:
            status = "fail"
            reason = "visual_regression"

        if status == "fail":
            failures += 1
            print(f"  FAIL: {reason} (diff={diff_score:.4f})")
        else:
            print(f"  PASS (diff={diff_score:.4f})")

        results.append(
            {
                "name": name,
                "status": status,
                "reason": reason,
                **metrics,
                "baseline_rel": os.path.relpath(baseline_path, output_dir),
                "current_rel": os.path.relpath(current_path, output_dir),
                "diff_rel": os.path.relpath(diff_path, output_dir),
                "diff_amplified_rel": os.path.relpath(diff_amp_path, output_dir),
            }
        )

    extra_currents = sorted(
        set(p.name for p in current_dir.glob("*.png"))
        - {p.name for p in baseline_images}
    )
    for extra in extra_currents:
        results.append(
            {
                "name": extra,
                "status": "fail",
                "reason": "unexpected_current",
            }
        )
        failures += 1

    summary = {
        "total": len(results),
        "passed": len([r for r in results if r["status"] == "pass"]),
        "failed": failures,
        "threshold": args.threshold,
        "background_threshold": args.background_threshold,
        "generated_at": datetime.utcnow().isoformat(timespec="seconds") + "Z",
    }

    output = {
        "summary": summary,
        "tests": results,
    }
    (output_dir / "results.json").write_text(
        json.dumps(output, indent=2, ensure_ascii=True),
        encoding="utf-8",
    )

    write_report(output_dir, output, args.threshold, args.background_threshold)

    if failures:
        print(f"FAIL: {failures} regression(s) detected.")
        return 1

    print("PASS: UI visual regression tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
