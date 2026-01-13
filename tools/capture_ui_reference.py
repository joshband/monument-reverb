#!/usr/bin/env python3
"""
Capture Monument UI screenshots for visual regression baselines.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
import time
import ctypes
from dataclasses import dataclass
from datetime import datetime
from pathlib import Path
from typing import Dict, List, Optional, Tuple


DEFAULT_DELAY = 0.6
DEFAULT_PROCESS_NAME = "Monument"
DEFAULT_WINDOW_METHOD = "auto"
DEFAULT_BUILD_DIR = "build"


@dataclass
class UIState:
    name: str
    label: Optional[str] = None
    delay: float = DEFAULT_DELAY
    coords: Optional[Tuple[float, float]] = None


DEFAULT_STATES: List[UIState] = [
    UIState(name="01_default"),
    UIState(name="02_base_params", label="BASE PARAMS"),
    UIState(name="03_modulation", label="MODULATION"),
    UIState(name="04_timeline", label="TIMELINE"),
]


def run_command(command: List[str]) -> subprocess.CompletedProcess:
    return subprocess.run(command, capture_output=True, text=True, check=False)


def run_osascript(script: str) -> str:
    result = run_command(["osascript", "-e", script])
    if result.returncode != 0:
        stderr = result.stderr.strip() or "osascript failed"
        raise RuntimeError(stderr)
    return result.stdout.strip()


def escape_applescript(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"')


def load_coregraphics():
    try:
        core_graphics = ctypes.cdll.LoadLibrary(
            "/System/Library/Frameworks/CoreGraphics.framework/CoreGraphics"
        )
        core_foundation = ctypes.cdll.LoadLibrary(
            "/System/Library/Frameworks/CoreFoundation.framework/CoreFoundation"
        )
        return core_graphics, core_foundation
    except OSError:
        return None, None


def cfstring_to_py(core_foundation, value: ctypes.c_void_p) -> str:
    if not value:
        return ""
    k_cf_string_encoding_utf8 = 0x08000100
    core_foundation.CFStringGetCStringPtr.argtypes = [ctypes.c_void_p, ctypes.c_uint32]
    core_foundation.CFStringGetCStringPtr.restype = ctypes.c_char_p
    ptr = core_foundation.CFStringGetCStringPtr(value, k_cf_string_encoding_utf8)
    if ptr:
        return ctypes.cast(ptr, ctypes.c_char_p).value.decode("utf-8")

    core_foundation.CFStringGetLength.argtypes = [ctypes.c_void_p]
    core_foundation.CFStringGetLength.restype = ctypes.c_long
    core_foundation.CFStringGetMaximumSizeForEncoding.argtypes = [
        ctypes.c_long,
        ctypes.c_uint32,
    ]
    core_foundation.CFStringGetMaximumSizeForEncoding.restype = ctypes.c_long
    core_foundation.CFStringGetCString.argtypes = [
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_long,
        ctypes.c_uint32,
    ]
    core_foundation.CFStringGetCString.restype = ctypes.c_bool

    length = core_foundation.CFStringGetLength(value)
    max_size = core_foundation.CFStringGetMaximumSizeForEncoding(
        length, k_cf_string_encoding_utf8
    )
    buffer = ctypes.create_string_buffer(max_size + 1)
    ok = core_foundation.CFStringGetCString(
        value, buffer, max_size + 1, k_cf_string_encoding_utf8
    )
    if not ok:
        return ""
    return buffer.value.decode("utf-8")


def cfnumber_to_int(core_foundation, value: ctypes.c_void_p) -> int:
    if not value:
        return 0
    k_cf_number_sint64 = 4
    result = ctypes.c_longlong()
    core_foundation.CFNumberGetValue.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_void_p,
    ]
    core_foundation.CFNumberGetValue.restype = ctypes.c_bool
    core_foundation.CFNumberGetValue(value, k_cf_number_sint64, ctypes.byref(result))
    return int(result.value)


def cfnumber_to_float(core_foundation, value: ctypes.c_void_p) -> float:
    if not value:
        return 0.0
    k_cf_number_double = 13
    result = ctypes.c_double()
    core_foundation.CFNumberGetValue.argtypes = [
        ctypes.c_void_p,
        ctypes.c_int,
        ctypes.c_void_p,
    ]
    core_foundation.CFNumberGetValue.restype = ctypes.c_bool
    core_foundation.CFNumberGetValue(value, k_cf_number_double, ctypes.byref(result))
    return float(result.value)


def cfbool_to_bool(core_foundation, value: ctypes.c_void_p) -> bool:
    if not value:
        return False
    core_foundation.CFBooleanGetValue.argtypes = [ctypes.c_void_p]
    core_foundation.CFBooleanGetValue.restype = ctypes.c_bool
    return bool(core_foundation.CFBooleanGetValue(value))


def resolve_build_dir(project_root: Path, build_dir: Optional[str]) -> Path:
    if build_dir:
        resolved = Path(build_dir)
        if not resolved.is_absolute():
            resolved = project_root / resolved
        return resolved
    default = project_root / DEFAULT_BUILD_DIR
    ninja = project_root / "build-ninja"
    if default.exists():
        return default
    if ninja.exists():
        return ninja
    return default


def find_standalone_app(build_dir: Path, test_config: str) -> Optional[Path]:
    base = build_dir / "Monument_artefacts"
    candidates = [
        base / test_config / "Standalone" / "Monument.app",
        base / "Debug" / "Standalone" / "Monument.app",
        base / "Release" / "Standalone" / "Monument.app",
        base / "RelWithDebInfo" / "Standalone" / "Monument.app",
        base / "Standalone" / "Monument.app",
    ]
    for candidate in candidates:
        if candidate.exists():
            return candidate
    return None


def launch_app(app_path: Path) -> None:
    resolved = app_path.resolve()
    result = run_command(["open", str(resolved)])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "Failed to launch app")


def set_frontmost(process_name: str) -> None:
    escaped = escape_applescript(process_name)
    script = f'''
tell application "System Events"
  if not (exists process "{escaped}") then return ""
  tell process "{escaped}"
    set frontmost to true
  end tell
end tell
'''
    run_osascript(script)


def get_window_info(process_name: str) -> Optional[Dict[str, int]]:
    escaped = escape_applescript(process_name)
    script = f'''
tell application "System Events"
  if not (exists process "{escaped}") then return ""
  tell process "{escaped}"
    if (count of windows) = 0 then return ""
    set win to window 1
    set pos to position of win
    set size to size of win
    set winId to id of win
    return (item 1 of pos) & "," & (item 2 of pos) & "," & (item 1 of size) & "," & (item 2 of size) & "," & winId
  end tell
end tell
'''
    output = run_osascript(script)
    if not output:
        return None
    parts = output.split(",")
    if len(parts) != 5:
        return None
    try:
        x, y, width, height, win_id = [int(p) for p in parts]
    except ValueError:
        return None
    return {
        "x": x,
        "y": y,
        "width": width,
        "height": height,
        "id": win_id,
    }


def get_window_info_cg(process_name: str) -> Optional[Dict[str, int]]:
    core_graphics, core_foundation = load_coregraphics()
    if not core_graphics or not core_foundation:
        return None

    core_graphics.CGWindowListCopyWindowInfo.argtypes = [
        ctypes.c_uint32,
        ctypes.c_uint32,
    ]
    core_graphics.CGWindowListCopyWindowInfo.restype = ctypes.c_void_p

    core_foundation.CFArrayGetCount.argtypes = [ctypes.c_void_p]
    core_foundation.CFArrayGetCount.restype = ctypes.c_long
    core_foundation.CFArrayGetValueAtIndex.argtypes = [
        ctypes.c_void_p,
        ctypes.c_long,
    ]
    core_foundation.CFArrayGetValueAtIndex.restype = ctypes.c_void_p
    core_foundation.CFDictionaryGetValue.argtypes = [
        ctypes.c_void_p,
        ctypes.c_void_p,
    ]
    core_foundation.CFDictionaryGetValue.restype = ctypes.c_void_p
    core_foundation.CFStringCreateWithCString.argtypes = [
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.c_uint32,
    ]
    core_foundation.CFStringCreateWithCString.restype = ctypes.c_void_p
    core_foundation.CFRelease.argtypes = [ctypes.c_void_p]
    core_foundation.CFRelease.restype = None

    def cfkey(name: str) -> ctypes.c_void_p:
        k_cf_string_encoding_utf8 = 0x08000100
        return core_foundation.CFStringCreateWithCString(
            None, name.encode("utf-8"), k_cf_string_encoding_utf8
        )

    key_owner = cfkey("kCGWindowOwnerName")
    key_window = cfkey("kCGWindowName")
    key_number = cfkey("kCGWindowNumber")
    key_bounds = cfkey("kCGWindowBounds")
    key_onscreen = cfkey("kCGWindowIsOnscreen")
    key_layer = cfkey("kCGWindowLayer")
    key_x = cfkey("X")
    key_y = cfkey("Y")
    key_w = cfkey("Width")
    key_h = cfkey("Height")

    window_list = core_graphics.CGWindowListCopyWindowInfo(1, 0)
    if not window_list:
        return None

    try:
        count = core_foundation.CFArrayGetCount(window_list)
        candidates = []
        for idx in range(count):
            entry = core_foundation.CFArrayGetValueAtIndex(window_list, idx)
            owner = cfstring_to_py(
                core_foundation,
                core_foundation.CFDictionaryGetValue(entry, key_owner),
            )
            if owner != process_name:
                continue
            window_number = cfnumber_to_int(
                core_foundation,
                core_foundation.CFDictionaryGetValue(entry, key_number),
            )
            layer = cfnumber_to_int(
                core_foundation,
                core_foundation.CFDictionaryGetValue(entry, key_layer),
            )
            onscreen = cfbool_to_bool(
                core_foundation,
                core_foundation.CFDictionaryGetValue(entry, key_onscreen),
            )
            bounds = core_foundation.CFDictionaryGetValue(entry, key_bounds)
            if not bounds:
                continue
            x = cfnumber_to_float(
                core_foundation,
                core_foundation.CFDictionaryGetValue(bounds, key_x),
            )
            y = cfnumber_to_float(
                core_foundation,
                core_foundation.CFDictionaryGetValue(bounds, key_y),
            )
            width = cfnumber_to_float(
                core_foundation,
                core_foundation.CFDictionaryGetValue(bounds, key_w),
            )
            height = cfnumber_to_float(
                core_foundation,
                core_foundation.CFDictionaryGetValue(bounds, key_h),
            )
            window_name = cfstring_to_py(
                core_foundation,
                core_foundation.CFDictionaryGetValue(entry, key_window),
            )

            if not onscreen or layer != 0:
                continue
            if width <= 0 or height <= 0:
                continue

            candidates.append(
                {
                    "id": window_number,
                    "x": int(round(x)),
                    "y": int(round(y)),
                    "width": int(round(width)),
                    "height": int(round(height)),
                    "name": window_name,
                }
            )

        if not candidates:
            return None
        candidates.sort(key=lambda item: item["width"] * item["height"], reverse=True)
        best = candidates[0]
        return {
            "id": best["id"],
            "x": best["x"],
            "y": best["y"],
            "width": best["width"],
            "height": best["height"],
        }
    finally:
        core_foundation.CFRelease(window_list)


def wait_for_window(process_name: str, timeout: float) -> Dict[str, int]:
    deadline = time.time() + timeout
    while time.time() < deadline:
        info = get_window_info(process_name)
        if info:
            return info
        time.sleep(0.2)
    raise RuntimeError(f"Window for process '{process_name}' not found")


def resolve_window_info(process_name: str, timeout: float, method: str) -> Dict[str, int]:
    last_error: Optional[str] = None
    methods = []
    if method == "auto":
        methods = ["applescript", "cgwindow"]
    else:
        methods = [method]

    for current in methods:
        try:
            if current == "applescript":
                return wait_for_window(process_name, timeout)
            if current == "cgwindow":
                deadline = time.time() + timeout
                while time.time() < deadline:
                    info = get_window_info_cg(process_name)
                    if info:
                        return info
                    time.sleep(0.2)
                raise RuntimeError("CGWindow list did not return a window")
        except RuntimeError as exc:
            last_error = str(exc)

    raise RuntimeError(last_error or "Unable to resolve window info")


def click_button(process_name: str, label: str) -> bool:
    escaped_process = escape_applescript(process_name)
    escaped_label = escape_applescript(label)
    script = f'''
tell application "System Events"
  if not (exists process "{escaped_process}") then return "missing"
  tell process "{escaped_process}"
    set frontmost to true
    try
      click (first button whose name is "{escaped_label}")
      return "ok"
    on error
      try
        click (first button whose title is "{escaped_label}")
        return "ok"
      on error
        return "missing"
      end try
    end try
  end tell
end tell
'''
    output = run_osascript(script)
    return output.strip() == "ok"


def click_coords(window_info: Dict[str, int], coords: Tuple[float, float]) -> bool:
    try:
        import pyautogui  # type: ignore
    except ImportError:
        return False
    x = int(window_info["x"] + window_info["width"] * coords[0])
    y = int(window_info["y"] + window_info["height"] * coords[1])
    pyautogui.click(x, y)
    return True


def capture_window(
    mode: str,
    window_info: Dict[str, int],
    output_path: Path,
) -> None:
    if mode == "screencapture":
        result = run_command(
            [
                "screencapture",
                "-x",
                "-o",
                "-l",
                str(window_info["id"]),
                str(output_path),
            ]
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr.strip() or "screencapture failed")
        return

    if mode == "pyautogui":
        try:
            import pyautogui  # type: ignore
        except ImportError as exc:
            raise RuntimeError("pyautogui not installed") from exc
        region = (
            window_info["x"],
            window_info["y"],
            window_info["width"],
            window_info["height"],
        )
        image = pyautogui.screenshot(region=region)
        image.save(output_path)
        return

    raise RuntimeError(f"Unknown capture mode: {mode}")


def capture_interactive(output_path: Path) -> None:
    if not shutil.which("screencapture"):
        raise RuntimeError("screencapture not available for manual capture")
    result = run_command(["screencapture", "-i", "-o", str(output_path)])
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "screencapture failed")


def analyze_image(image_path: Path) -> Dict[str, object]:
    try:
        from PIL import Image, ImageStat
    except ImportError as exc:
        raise RuntimeError("pillow not installed") from exc

    image = Image.open(image_path).convert("RGB")
    stat = ImageStat.Stat(image)
    mean = stat.mean
    avg_brightness = 0.2126 * mean[0] + 0.7152 * mean[1] + 0.0722 * mean[2]

    width, height = image.size
    sample = min(20, width, height)
    corners = {
        "top_left": (0, 0),
        "top_right": (width - sample, 0),
        "bottom_left": (0, height - sample),
        "bottom_right": (width - sample, height - sample),
    }
    corner_brightness = {}
    for name, (x, y) in corners.items():
        crop = image.crop((x, y, x + sample, y + sample))
        cstat = ImageStat.Stat(crop)
        cmean = cstat.mean
        corner_brightness[name] = round(
            0.2126 * cmean[0] + 0.7152 * cmean[1] + 0.0722 * cmean[2],
            2,
        )

    theme = "dark" if avg_brightness < 80.0 else "light"

    return {
        "width": width,
        "height": height,
        "avg_rgb": [round(v, 2) for v in mean],
        "avg_brightness": round(avg_brightness, 2),
        "corner_brightness": corner_brightness,
        "theme": theme,
    }


def write_index_html(output_dir: Path, metadata: Dict[str, object]) -> None:
    rows = []
    for item in metadata["states"]:
        analysis = item.get("analysis", {})
        rows.append(
            f"""
            <tr>
              <td>{item["name"]}</td>
              <td><img src="{item["image"]}" alt="{item["name"]}"></td>
              <td>{analysis.get("theme", "n/a")}</td>
              <td>{analysis.get("avg_brightness", "n/a")}</td>
              <td>{analysis.get("avg_rgb", "n/a")}</td>
              <td>{item.get("action_status", "n/a")}</td>
            </tr>
            """
        )

    html = f"""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Monument UI Capture Report</title>
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
    img {{
      max-width: 320px;
      height: auto;
      border: 1px solid #ccc;
    }}
  </style>
</head>
<body>
  <header>
    <h1>Monument UI Capture Report</h1>
    <div>
      Generated: {metadata["generated_at"]}<br>
      App: {metadata.get("app_path", "n/a")}<br>
      Capture Mode: {metadata.get("capture_mode", "n/a")}
    </div>
  </header>
  <table>
    <thead>
      <tr>
        <th>State</th>
        <th>Screenshot</th>
        <th>Theme</th>
        <th>Avg Brightness</th>
        <th>Avg RGB</th>
        <th>Action Status</th>
      </tr>
    </thead>
    <tbody>
      {''.join(rows)}
    </tbody>
  </table>
</body>
</html>
"""
    (output_dir / "index.html").write_text(html, encoding="utf-8")


def load_config(config_path: Path) -> Tuple[Optional[str], List[UIState]]:
    data = json.loads(config_path.read_text(encoding="utf-8"))
    process_name = data.get("process_name")
    states: List[UIState] = []
    for entry in data.get("states", []):
        name = entry["name"]
        label = entry.get("label")
        delay = float(entry.get("delay", DEFAULT_DELAY))
        coords = entry.get("coords")
        coords_tuple = None
        if coords is not None:
            coords_tuple = (float(coords[0]), float(coords[1]))
        states.append(UIState(name=name, label=label, delay=delay, coords=coords_tuple))
    if not states:
        states = DEFAULT_STATES
    return process_name, states


def pick_capture_mode(requested: str) -> str:
    if requested != "auto":
        return requested
    return "screencapture" if shutil.which("screencapture") else "pyautogui"


def main() -> int:
    parser = argparse.ArgumentParser(description="Capture Monument UI reference screenshots.")
    parser.add_argument(
        "--output-dir",
        type=Path,
        default=Path("test-results/ui-baseline"),
        help="Directory to write screenshots and reports.",
    )
    parser.add_argument(
        "--app",
        type=Path,
        default=None,
        help="Path to Monument.app (standalone).",
    )
    parser.add_argument(
        "--process-name",
        type=str,
        default=DEFAULT_PROCESS_NAME,
        help="Process name for the standalone app.",
    )
    parser.add_argument(
        "--test-config",
        type=str,
        default=os.environ.get("TEST_CONFIG", "Debug"),
        help="Build config for auto app lookup.",
    )
    parser.add_argument(
        "--build-dir",
        type=str,
        default=os.environ.get("BUILD_DIR"),
        help="Build directory for locating Monument.app (defaults to build/ or build-ninja/).",
    )
    parser.add_argument(
        "--config",
        type=Path,
        default=None,
        help="Optional JSON config defining UI states.",
    )
    parser.add_argument(
        "--state",
        action="append",
        default=[],
        help="Capture only specific state(s) by name.",
    )
    parser.add_argument(
        "--skip-launch",
        action="store_true",
        help="Skip launching the app (assumes it's already running).",
    )
    parser.add_argument(
        "--no-actions",
        action="store_true",
        help="Do not click any UI elements (capture current window state only).",
    )
    parser.add_argument(
        "--wait",
        type=float,
        default=10.0,
        help="Seconds to wait for the window to appear.",
    )
    parser.add_argument(
        "--capture-mode",
        choices=["auto", "screencapture", "pyautogui"],
        default="auto",
        help="Screen capture backend to use.",
    )
    parser.add_argument(
        "--window-method",
        choices=["auto", "applescript", "cgwindow"],
        default=DEFAULT_WINDOW_METHOD,
        help="Window detection method for automated capture.",
    )
    parser.add_argument(
        "--manual",
        action="store_true",
        help="Manual capture using interactive screencapture (no AppleScript).",
    )

    args = parser.parse_args()
    project_root = Path(__file__).resolve().parents[1]
    build_dir = resolve_build_dir(project_root, args.build_dir)
    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    config_path = args.config or os.environ.get("UI_CAPTURE_CONFIG")
    process_name = args.process_name
    states = DEFAULT_STATES

    if config_path:
        config_process, states = load_config(Path(config_path))
        if config_process:
            process_name = config_process

    if args.state:
        selected = set(args.state)
        states = [state for state in states if state.name in selected]
        if not states:
            print("ERROR: No matching states in config.", file=sys.stderr)
            return 2

    app_path = args.app
    if not app_path:
        app_path = find_standalone_app(build_dir, args.test_config)
    if not app_path or not app_path.exists():
        print("ERROR: Monument.app not found. Build the standalone target first.", file=sys.stderr)
        return 2

    capture_mode = pick_capture_mode(args.capture_mode)

    try:
        if not args.skip_launch:
            launch_app(app_path)
        if args.manual:
            window_info = {}
        else:
            window_info = resolve_window_info(process_name, args.wait, args.window_method)
            set_frontmost(process_name)
    except RuntimeError as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        print("Ensure Accessibility permissions allow Terminal/Python to control the UI.", file=sys.stderr)
        print("If Automation is blocked, rerun with --manual for interactive capture.", file=sys.stderr)
        return 2

    metadata: Dict[str, object] = {
        "generated_at": datetime.utcnow().isoformat(timespec="seconds") + "Z",
        "app_path": str(app_path),
        "process_name": process_name,
        "capture_mode": capture_mode,
        "window": window_info if window_info else None,
        "states": [],
    }

    click_window_info = window_info
    if not args.manual and not args.no_actions:
        if any(state.coords for state in states):
            try:
                click_window_info = get_window_info(process_name) or window_info
            except RuntimeError:
                click_window_info = window_info

    for state in states:
        action_status = "skipped"
        if args.manual:
            prompt = f"Set UI state '{state.name}'"
            if state.label:
                prompt += f" (label: {state.label})"
            prompt += " and press Enter to capture..."
            input(prompt)
        elif state.label and not args.no_actions:
            try:
                clicked = click_button(process_name, state.label)
            except RuntimeError:
                clicked = False
            if clicked:
                action_status = "clicked_label"
            elif state.coords:
                if click_coords(click_window_info, state.coords):
                    action_status = "clicked_coords"
                else:
                    action_status = "missing_target"
            else:
                action_status = "missing_target"

        if state.delay > 0:
            time.sleep(state.delay)

        image_name = f"{state.name}.png"
        image_path = output_dir / image_name
        try:
            if args.manual:
                capture_interactive(image_path)
            else:
                capture_window(capture_mode, window_info, image_path)
        except RuntimeError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 2

        try:
            analysis = analyze_image(image_path)
        except RuntimeError as exc:
            print(f"ERROR: {exc}", file=sys.stderr)
            return 2

        metadata["states"].append(
            {
                "name": state.name,
                "label": state.label,
                "image": image_name,
                "action_status": action_status,
                "analysis": analysis,
            }
        )

    (output_dir / "metadata.json").write_text(
        json.dumps(metadata, indent=2, ensure_ascii=True),
        encoding="utf-8",
    )
    write_index_html(output_dir, metadata)

    print(f"Captured {len(states)} UI state(s) to {output_dir}")
    print(f"Report: {output_dir / 'index.html'}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
