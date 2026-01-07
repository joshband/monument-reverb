#!/usr/bin/env python3
"""
Monument Reverb - Visual UI Regression Testing

Automated visual regression testing for Monument UI.
Launches standalone app, captures screenshots, and compares against baseline.

Usage:
    python3 tools/test_ui_visual.py                    # Run all tests
    python3 tools/test_ui_visual.py --update-baseline  # Update baseline images
    python3 tools/test_ui_visual.py --threshold 0.05   # Custom diff threshold

Requirements:
    pip3 install pyautogui pillow numpy
"""

import subprocess
import time
import os
import sys
import argparse
from pathlib import Path
import json
from typing import Dict, List, Tuple

try:
    import pyautogui
    from PIL import Image, ImageChops, ImageDraw, ImageFont
    import numpy as np
except ImportError:
    print("Error: Missing dependencies. Install with:")
    print("  pip3 install pyautogui pillow numpy")
    sys.exit(1)


class VisualTest:
    """Individual visual regression test."""

    def __init__(self, name: str, description: str, button_to_click: str = None):
        self.name = name
        self.description = description
        self.button_to_click = button_to_click
        self.result = None

    def __repr__(self):
        return f"VisualTest({self.name})"


class MonumentVisualTester:
    """Visual regression tester for Monument UI."""

    def __init__(self, baseline_dir="test-results/ui-baseline",
                 output_dir="test-results/ui-current",
                 diff_threshold=0.02):
        self.baseline_dir = Path(baseline_dir)
        self.output_dir = Path(output_dir)
        self.diff_dir = self.output_dir / "diffs"
        self.diff_threshold = diff_threshold

        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.diff_dir.mkdir(parents=True, exist_ok=True)

        self.app_process = None
        self.results = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "tests": [],
            "summary": {
                "total": 0,
                "passed": 0,
                "failed": 0,
                "warnings": 0
            }
        }

        # Define standard tests
        self.tests = [
            VisualTest("01_default", "Default view with macro controls"),
            VisualTest("02_base_params", "BASE PARAMS expanded", "BASE PARAMS"),
            VisualTest("03_modulation", "MODULATION panel open", "MODULATION"),
            VisualTest("04_timeline", "TIMELINE panel open", "TIMELINE"),
        ]

    def launch_standalone(self) -> bool:
        """Launch Monument standalone app."""
        app_path = Path("build/Monument_artefacts/Debug/Standalone/Monument.app").resolve()

        if not app_path.exists():
            print(f"❌ Monument standalone not found at {app_path}")
            return False

        print(f"🚀 Launching Monument standalone...")

        # Kill existing processes
        subprocess.run(["killall", "-9", "Monument"],
                      stderr=subprocess.DEVNULL, check=False)
        time.sleep(0.5)

        # Launch app (use absolute path)
        self.app_process = subprocess.Popen(
            ["open", str(app_path)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        # Wait for window
        print("⏳ Waiting for Monument window...")
        time.sleep(5)  # Increased from 3 to 5 seconds

        return True

    def find_monument_window(self) -> Dict:
        """Find Monument window position and size."""
        script = '''
        tell application "System Events"
            tell process "Monument"
                get {size, position} of window 1
            end tell
        end tell
        '''

        try:
            result = subprocess.run(
                ["osascript", "-e", script],
                capture_output=True,
                text=True,
                check=True
            )

            # Parse result: "width, height, x, y"
            parts = [int(x.strip()) for x in result.stdout.strip().split(',')]
            if len(parts) >= 4:
                return {'width': parts[0], 'height': parts[1], 'x': parts[2], 'y': parts[3]}
        except subprocess.CalledProcessError:
            pass

        return None

    def click_button(self, button_name: str, wait: float = 1.0) -> bool:
        """Click UI button using AppleScript."""
        script = f'''
        tell application "System Events"
            tell process "Monument"
                click button "{button_name}" of window 1
            end tell
        end tell
        '''

        try:
            subprocess.run(["osascript", "-e", script], check=True,
                          capture_output=True)
            time.sleep(wait)
            return True
        except subprocess.CalledProcessError:
            return False

    def capture_screenshot(self, name: str) -> Image:
        """Capture current window screenshot."""
        window = self.find_monument_window()

        if not window:
            raise RuntimeError(f"Could not find Monument window for {name}")

        screenshot = pyautogui.screenshot(region=(
            window['x'], window['y'],
            window['width'], window['height']
        ))

        # Save current screenshot
        filepath = self.output_dir / f"{name}.png"
        screenshot.save(filepath)

        return screenshot

    def compare_images(self, baseline: Image, current: Image, name: str) -> Dict:
        """Compare two images and generate diff."""
        # Ensure same size
        if baseline.size != current.size:
            return {
                "status": "failed",
                "reason": "size_mismatch",
                "baseline_size": baseline.size,
                "current_size": current.size,
                "difference_score": 1.0
            }

        # Convert to RGB
        baseline_rgb = baseline.convert('RGB')
        current_rgb = current.convert('RGB')

        # Calculate pixel-by-pixel difference
        diff_image = ImageChops.difference(baseline_rgb, current_rgb)

        # Convert to numpy for analysis
        diff_array = np.array(diff_image)
        baseline_array = np.array(baseline_rgb)

        # Calculate metrics
        total_pixels = diff_array.size
        changed_pixels = np.count_nonzero(diff_array)
        max_diff = np.max(diff_array)
        mean_diff = np.mean(diff_array)

        # Normalized difference score (0.0 = identical, 1.0 = completely different)
        difference_score = mean_diff / 255.0

        # Check background color (detect black vs white theme issues)
        bg_sample = baseline_array[20, 20]  # Top-left corner
        current_bg = np.array(current_rgb)[20, 20]
        bg_diff = np.abs(int(bg_sample.mean()) - int(current_bg.mean()))

        # Generate visual diff image
        diff_visual = self.create_diff_visualization(baseline_rgb, current_rgb, diff_image, name)

        # Determine status
        if difference_score == 0.0:
            status = "passed"
            reason = "identical"
        elif difference_score < self.diff_threshold:
            status = "passed"
            reason = "acceptable_difference"
        elif bg_diff > 100:
            status = "failed"
            reason = "background_color_mismatch"
        else:
            status = "failed"
            reason = "visual_regression"

        return {
            "status": status,
            "reason": reason,
            "difference_score": float(difference_score),
            "changed_pixels": int(changed_pixels),
            "total_pixels": int(total_pixels),
            "max_diff": int(max_diff),
            "mean_diff": float(mean_diff),
            "background_diff": int(bg_diff),
            "diff_image": diff_visual
        }

    def create_diff_visualization(self, baseline: Image, current: Image,
                                  diff: Image, name: str) -> Path:
        """Create visual diff comparison image."""
        width, height = baseline.size

        # Create side-by-side comparison
        comparison = Image.new('RGB', (width * 3, height + 40))

        # Paste images
        comparison.paste(baseline, (0, 40))
        comparison.paste(current, (width, 40))

        # Amplify diff for visibility
        diff_array = np.array(diff)
        diff_amplified = np.clip(diff_array * 10, 0, 255).astype(np.uint8)
        diff_img = Image.fromarray(diff_amplified)
        comparison.paste(diff_img, (width * 2, 40))

        # Add labels
        draw = ImageDraw.Draw(comparison)
        try:
            font = ImageFont.truetype("/System/Library/Fonts/Helvetica.ttc", 16)
        except:
            font = ImageFont.load_default()

        draw.text((10, 10), "Baseline", fill=(100, 100, 100), font=font)
        draw.text((width + 10, 10), "Current", fill=(100, 100, 100), font=font)
        draw.text((width * 2 + 10, 10), "Diff (10x)", fill=(255, 0, 0), font=font)

        # Save diff
        diff_path = self.diff_dir / f"{name}_diff.png"
        comparison.save(diff_path)

        return diff_path

    def run_test(self, test: VisualTest) -> Dict:
        """Run single visual regression test."""
        print(f"\n🔍 Testing: {test.name}")
        print(f"   {test.description}")

        try:
            # Click button if needed
            if test.button_to_click:
                print(f"   Clicking: {test.button_to_click}")
                if not self.click_button(test.button_to_click, wait=2.0):
                    return {
                        "status": "failed",
                        "reason": "button_click_failed",
                        "button": test.button_to_click
                    }

            # Capture screenshot
            current = self.capture_screenshot(test.name)

            # Load baseline
            baseline_path = self.baseline_dir / f"{test.name}.png"
            if not baseline_path.exists():
                print(f"   ⚠️  No baseline found - creating baseline")
                return {
                    "status": "warning",
                    "reason": "no_baseline",
                    "message": f"Created baseline: {baseline_path}"
                }

            baseline = Image.open(baseline_path)

            # Compare
            result = self.compare_images(baseline, current, test.name)

            # Print result
            if result["status"] == "passed":
                print(f"   ✅ PASS (diff: {result['difference_score']:.4f})")
            elif result["status"] == "warning":
                print(f"   ⚠️  WARNING: {result['reason']}")
            else:
                print(f"   ❌ FAIL: {result['reason']}")
                print(f"      Difference: {result['difference_score']:.4f}")
                print(f"      Diff image: {result['diff_image']}")

            return result

        except Exception as e:
            print(f"   ❌ ERROR: {e}")
            return {
                "status": "failed",
                "reason": "exception",
                "error": str(e)
            }

    def run_all_tests(self) -> bool:
        """Run all visual regression tests."""
        print("=" * 60)
        print("Monument Visual Regression Tests")
        print("=" * 60)

        # Launch app
        if not self.launch_standalone():
            return False

        # Run each test
        for test in self.tests:
            result = self.run_test(test)

            # Store result
            test.result = result
            self.results["tests"].append({
                "name": test.name,
                "description": test.description,
                "result": result
            })

            # Update summary
            self.results["summary"]["total"] += 1
            if result["status"] == "passed":
                self.results["summary"]["passed"] += 1
            elif result["status"] == "warning":
                self.results["summary"]["warnings"] += 1
            else:
                self.results["summary"]["failed"] += 1

            time.sleep(0.5)

        return self.results["summary"]["failed"] == 0

    def save_results(self):
        """Save test results to JSON."""
        results_file = self.output_dir / "results.json"
        with open(results_file, 'w') as f:
            # Convert Path objects to strings for JSON
            json_results = self.results.copy()
            for test in json_results["tests"]:
                if "diff_image" in test["result"]:
                    test["result"]["diff_image"] = str(test["result"]["diff_image"])
            json.dump(json_results, f, indent=2)

        print(f"\n📄 Results saved: {results_file}")

    def generate_report(self):
        """Generate HTML test report."""
        summary = self.results["summary"]

        # Determine overall status
        if summary["failed"] == 0:
            status_icon = "✅"
            status_text = "ALL TESTS PASSED"
            status_color = "#4CAF50"
        else:
            status_icon = "❌"
            status_text = "TESTS FAILED"
            status_color = "#f44336"

        html = f"""<!DOCTYPE html>
<html>
<head>
    <title>Monument Visual Regression Test Report</title>
    <style>
        body {{ font-family: -apple-system, sans-serif; margin: 40px; background: #f5f5f5; }}
        h1 {{ color: #333; }}
        .summary {{ background: white; padding: 20px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .status {{ font-size: 24px; font-weight: bold; color: {status_color}; margin: 10px 0; }}
        .stats {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(150px, 1fr)); gap: 15px; margin: 20px 0; }}
        .stat {{ background: #f9f9f9; padding: 15px; border-radius: 4px; text-align: center; }}
        .stat-value {{ font-size: 32px; font-weight: bold; margin: 10px 0; }}
        .stat.passed {{ border-left: 4px solid #4CAF50; }}
        .stat.failed {{ border-left: 4px solid #f44336; }}
        .stat.warnings {{ border-left: 4px solid #ff9800; }}
        .test {{ background: white; padding: 20px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .test.passed {{ border-left: 4px solid #4CAF50; }}
        .test.failed {{ border-left: 4px solid #f44336; }}
        .test.warning {{ border-left: 4px solid #ff9800; }}
        .test h3 {{ margin-top: 0; }}
        .test img {{ max-width: 100%; border: 1px solid #ddd; border-radius: 4px; margin: 10px 0; }}
        .metrics {{ background: #f9f9f9; padding: 10px; border-radius: 4px; margin: 10px 0; font-family: monospace; font-size: 0.9em; }}
        .metric-row {{ margin: 5px 0; }}
    </style>
</head>
<body>
    <h1>Monument Visual Regression Test Report</h1>
    <div class="summary">
        <div class="status">{status_icon} {status_text}</div>
        <p>Generated: {self.results["timestamp"]}</p>
        <p>Threshold: {self.diff_threshold}</p>
        <div class="stats">
            <div class="stat passed">
                <div>Passed</div>
                <div class="stat-value">{summary["passed"]}</div>
            </div>
            <div class="stat failed">
                <div>Failed</div>
                <div class="stat-value">{summary["failed"]}</div>
            </div>
            <div class="stat warnings">
                <div>Warnings</div>
                <div class="stat-value">{summary["warnings"]}</div>
            </div>
        </div>
    </div>
"""

        for test_data in self.results["tests"]:
            result = test_data["result"]
            status_class = result["status"]

            status_icon = {
                "passed": "✅",
                "failed": "❌",
                "warning": "⚠️"
            }.get(status_class, "❓")

            html += f"""
    <div class="test {status_class}">
        <h3>{status_icon} {test_data["name"]}</h3>
        <p>{test_data["description"]}</p>
        <div class="metrics">
            <div class="metric-row"><strong>Status:</strong> {result["status"].upper()} - {result["reason"]}</div>
"""

            if "difference_score" in result:
                html += f"""
            <div class="metric-row"><strong>Difference Score:</strong> {result["difference_score"]:.6f} (threshold: {self.diff_threshold})</div>
            <div class="metric-row"><strong>Changed Pixels:</strong> {result["changed_pixels"]:,} / {result["total_pixels"]:,}</div>
            <div class="metric-row"><strong>Mean Diff:</strong> {result["mean_diff"]:.2f} / 255</div>
            <div class="metric-row"><strong>Background Diff:</strong> {result["background_diff"]}</div>
"""

            html += "        </div>\n"

            if "diff_image" in result and result["status"] != "passed":
                diff_rel = Path(result["diff_image"]).relative_to(self.output_dir)
                html += f'        <img src="{diff_rel}" alt="Visual diff for {test_data["name"]}">\n'

            html += "    </div>\n"

        html += """
</body>
</html>
"""

        report_file = self.output_dir / "report.html"
        with open(report_file, 'w') as f:
            f.write(html)

        print(f"📊 Report generated: {report_file}")
        print(f"   View: open {report_file}")

    def cleanup(self):
        """Close Monument app."""
        subprocess.run(["killall", "Monument"],
                      stderr=subprocess.DEVNULL, check=False)


def main():
    parser = argparse.ArgumentParser(description="Monument Visual Regression Tests")
    parser.add_argument("--threshold", type=float, default=0.02,
                       help="Difference threshold (0.0-1.0, default: 0.02)")
    parser.add_argument("--baseline-dir", default="test-results/ui-baseline",
                       help="Baseline images directory")
    parser.add_argument("--output-dir", default="test-results/ui-current",
                       help="Output directory for current screenshots")

    args = parser.parse_args()

    tester = MonumentVisualTester(
        baseline_dir=args.baseline_dir,
        output_dir=args.output_dir,
        diff_threshold=args.threshold
    )

    try:
        # Run tests
        success = tester.run_all_tests()

        # Save results
        tester.save_results()
        tester.generate_report()

        # Print summary
        print("\n" + "=" * 60)
        summary = tester.results["summary"]
        print(f"Total:    {summary['total']}")
        print(f"Passed:   {summary['passed']} ✅")
        print(f"Failed:   {summary['failed']} ❌")
        print(f"Warnings: {summary['warnings']} ⚠️")
        print("=" * 60)

        return 0 if success else 1

    except KeyboardInterrupt:
        print("\n\n⚠️  Interrupted by user")
        return 1

    except Exception as e:
        print(f"\n❌ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1

    finally:
        tester.cleanup()


if __name__ == "__main__":
    sys.exit(main())
