#!/usr/bin/env python3
"""
Monument Reverb - UI Reference Capture Tool

Captures baseline screenshots of the Monument standalone app for visual regression testing.
This tool helps create reference images for different UI states.

Usage:
    python3 tools/capture_ui_reference.py

Requirements:
    pip3 install pyautogui pillow
"""

import subprocess
import time
import os
import sys
from pathlib import Path
import json

try:
    import pyautogui
    from PIL import Image
except ImportError:
    print("Error: Missing dependencies. Install with:")
    print("  pip3 install pyautogui pillow")
    sys.exit(1)


class MonumentUICapture:
    """Captures Monument UI screenshots for baseline reference."""

    def __init__(self, output_dir="test-results/ui-baseline"):
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        self.app_process = None
        self.metadata = {
            "captures": [],
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "version": "1.0.0"
        }

    def launch_standalone(self):
        """Launch Monument standalone app."""
        app_path = Path("build/Monument_artefacts/Debug/Standalone/Monument.app").resolve()

        if not app_path.exists():
            print(f"Error: Monument standalone not found at {app_path}")
            print("Build it first with: cmake --build build --target Monument_Standalone")
            return False

        print(f"Launching Monument standalone from {app_path}...")

        # Kill any existing Monument processes
        subprocess.run(["killall", "-9", "Monument"],
                      stderr=subprocess.DEVNULL, check=False)
        time.sleep(0.5)

        # Launch app (use absolute path)
        self.app_process = subprocess.Popen(
            ["open", str(app_path)],
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL
        )

        # Wait for app to launch and window to appear
        print("Waiting for Monument window to appear...")
        time.sleep(5)  # Increased from 3 to 5 seconds

        return True

    def find_monument_window(self):
        """Find Monument window using AppleScript."""
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
                return {
                    'width': parts[0],
                    'height': parts[1],
                    'x': parts[2],
                    'y': parts[3]
                }
        except subprocess.CalledProcessError:
            pass

        return None

    def capture_window(self, name, description=""):
        """Capture screenshot of Monument window."""
        window = self.find_monument_window()

        if not window:
            print(f"Warning: Could not find Monument window for '{name}'")
            return False

        print(f"Capturing: {name}")
        print(f"  Window: {window['width']}x{window['height']} at ({window['x']}, {window['y']})")

        # Capture screenshot
        screenshot = pyautogui.screenshot(region=(
            window['x'],
            window['y'],
            window['width'],
            window['height']
        ))

        # Save image
        filename = f"{name}.png"
        filepath = self.output_dir / filename
        screenshot.save(filepath)

        # Analyze image
        analysis = self.analyze_screenshot(screenshot)

        # Store metadata
        self.metadata["captures"].append({
            "name": name,
            "description": description,
            "filename": filename,
            "window": window,
            "analysis": analysis
        })

        print(f"  Saved: {filepath}")
        print(f"  Analysis: {analysis}")

        return True

    def analyze_screenshot(self, image):
        """Analyze screenshot for basic metrics."""
        # Convert to RGB if needed
        if image.mode != 'RGB':
            image = image.convert('RGB')

        # Sample colors from key areas
        width, height = image.size

        # Sample center region
        center_x, center_y = width // 2, height // 2
        center_color = image.getpixel((center_x, center_y))

        # Sample top-left corner (background)
        bg_color = image.getpixel((20, 20))

        # Calculate average brightness
        pixels = list(image.getdata())
        avg_brightness = sum(sum(p) for p in pixels) / (len(pixels) * 3)

        # Detect if background is dark or light
        bg_brightness = sum(bg_color) / 3
        is_dark = bg_brightness < 128

        return {
            "width": width,
            "height": height,
            "background_color": bg_color,
            "background_brightness": int(bg_brightness),
            "is_dark_theme": is_dark,
            "average_brightness": int(avg_brightness),
            "center_sample": center_color
        }

    def click_button(self, button_name, wait=1.0):
        """Click a button using AppleScript."""
        script = f'''
        tell application "System Events"
            tell process "Monument"
                click button "{button_name}" of window 1
            end tell
        end tell
        '''

        try:
            subprocess.run(
                ["osascript", "-e", script],
                check=True,
                capture_output=True
            )
            time.sleep(wait)
            return True
        except subprocess.CalledProcessError:
            print(f"Warning: Could not click button '{button_name}'")
            return False

    def capture_standard_states(self):
        """Capture all standard UI states."""
        captures = [
            ("01_default", "Default view with macro controls"),
            ("02_base_params", "With BASE PARAMS expanded", "BASE PARAMS"),
            ("03_modulation", "With MODULATION panel open", "MODULATION"),
            ("04_timeline", "With TIMELINE panel open", "TIMELINE"),
        ]

        for name, description, *button in captures:
            if button:
                print(f"\nExpanding: {button[0]}")
                self.click_button(button[0], wait=2.0)

            self.capture_window(name, description)
            time.sleep(0.5)

    def save_metadata(self):
        """Save capture metadata to JSON."""
        metadata_file = self.output_dir / "metadata.json"
        with open(metadata_file, 'w') as f:
            json.dump(self.metadata, f, indent=2)

        print(f"\nMetadata saved: {metadata_file}")

    def generate_report(self):
        """Generate HTML report of captured images."""
        html = """<!DOCTYPE html>
<html>
<head>
    <title>Monument UI Baseline Reference</title>
    <style>
        body {{ font-family: -apple-system, sans-serif; margin: 40px; background: #f5f5f5; }}
        h1 {{ color: #333; }}
        .capture {{ background: white; padding: 20px; margin: 20px 0; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }}
        .capture h2 {{ margin-top: 0; color: #555; }}
        .capture img {{ max-width: 100%; border: 1px solid #ddd; border-radius: 4px; }}
        .metadata {{ background: #f9f9f9; padding: 10px; margin: 10px 0; border-radius: 4px; font-size: 0.9em; }}
        .metadata code {{ background: #e0e0e0; padding: 2px 4px; border-radius: 2px; }}
        .analysis {{ display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 10px; }}
        .metric {{ background: white; padding: 8px; border-radius: 4px; border-left: 3px solid #4CAF50; }}
        .metric strong {{ display: block; color: #666; font-size: 0.85em; }}
    </style>
</head>
<body>
    <h1>Monument UI Baseline Reference</h1>
    <p>Generated: {timestamp}</p>
    <p>Version: {version}</p>
""".format(**self.metadata)

        for capture in self.metadata["captures"]:
            analysis = capture["analysis"]
            window = capture["window"]

            # Color display
            bg_color = f"rgb{analysis['background_color']}"
            theme = "🌙 Dark Theme" if analysis['is_dark_theme'] else "☀️ Light Theme"

            html += f"""
    <div class="capture">
        <h2>{capture['name']}</h2>
        <p>{capture['description']}</p>
        <img src="{capture['filename']}" alt="{capture['name']}">
        <div class="metadata">
            <strong>Window:</strong> <code>{window['width']}×{window['height']}</code> at
            <code>({window['x']}, {window['y']})</code>
        </div>
        <div class="analysis">
            <div class="metric">
                <strong>Theme</strong>
                {theme}
            </div>
            <div class="metric">
                <strong>Background Color</strong>
                <span style="display: inline-block; width: 20px; height: 20px; background: {bg_color}; border: 1px solid #ccc; border-radius: 3px; vertical-align: middle;"></span>
                <code>{bg_color}</code>
            </div>
            <div class="metric">
                <strong>Background Brightness</strong>
                {analysis['background_brightness']}/255
            </div>
            <div class="metric">
                <strong>Average Brightness</strong>
                {analysis['average_brightness']}/255
            </div>
        </div>
    </div>
"""

        html += """
</body>
</html>
"""

        report_file = self.output_dir / "index.html"
        with open(report_file, 'w') as f:
            f.write(html)

        print(f"Report generated: {report_file}")
        print(f"\nView report: open {report_file}")

    def cleanup(self):
        """Close Monument app."""
        print("\nClosing Monument...")
        subprocess.run(["killall", "Monument"],
                      stderr=subprocess.DEVNULL, check=False)


def main():
    print("=" * 60)
    print("Monument UI Reference Capture Tool")
    print("=" * 60)

    capture = MonumentUICapture()

    try:
        # Launch app
        if not capture.launch_standalone():
            return 1

        # Capture standard states
        print("\nCapturing standard UI states...")
        capture.capture_standard_states()

        # Save results
        capture.save_metadata()
        capture.generate_report()

        print("\n" + "=" * 60)
        print("✓ Capture complete!")
        print("=" * 60)
        print(f"Images saved: {capture.output_dir}")
        print(f"Captured: {len(capture.metadata['captures'])} states")

        return 0

    except KeyboardInterrupt:
        print("\n\nInterrupted by user")
        return 1

    except Exception as e:
        print(f"\nError: {e}")
        import traceback
        traceback.print_exc()
        return 1

    finally:
        capture.cleanup()


if __name__ == "__main__":
    sys.exit(main())
