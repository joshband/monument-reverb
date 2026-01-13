#!/usr/bin/env python3
"""
Extract layered knob assets from Line 6 knob renders on white backgrounds.
Outputs per-variant plate + knob layers with alpha for JUCE compositing.
"""
from __future__ import annotations

import argparse
import json
import math
import re
from collections import deque
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter
import numpy as np


DEFAULT_INPUT_DIR = Path("~/Desktop/Line 6 Delay/knobs").expanduser()
DEFAULT_OUTPUT_DIR = Path("assets/ui/line6")


NAME_MAP = {
    "f9497348-8e22-497f-9bdc-b1b70f0bc4e3": "line6_brass",
    "image": "line6_brass_alt",
}


def slugify(name: str, prefix: str) -> str:
    base = name.strip().lower()
    base = re.sub(r"[^a-z0-9]+", "_", base)
    base = base.strip("_")
    if prefix.startswith("line6") and base in NAME_MAP:
        return NAME_MAP[base]
    if prefix:
        if base.startswith(prefix):
            return base
        return f"{prefix}{base}"
    return base


def parse_hex_color(value: str) -> tuple[int, int, int]:
    if value.startswith("#"):
        value = value[1:]
    if len(value) == 6:
        return (int(value[0:2], 16), int(value[2:4], 16), int(value[4:6], 16))
    raise ValueError(f"Invalid hex color: {value}")


def estimate_background_color(arr: np.ndarray) -> np.ndarray:
    h, w, _ = arr.shape
    sample = 20
    corners = [
        arr[:sample, :sample, :],
        arr[:sample, w - sample :, :],
        arr[h - sample :, :sample, :],
        arr[h - sample :, w - sample :, :],
    ]
    stacked = np.concatenate([c.reshape(-1, 3) for c in corners], axis=0)
    return np.median(stacked, axis=0)


def dilate(mask: np.ndarray, iterations: int = 1) -> np.ndarray:
    padded = mask
    for _ in range(iterations):
        expanded = padded.copy()
        for dx in (-1, 0, 1):
            for dy in (-1, 0, 1):
                if dx == 0 and dy == 0:
                    continue
                shifted = np.roll(np.roll(padded, dx, axis=0), dy, axis=1)
                expanded |= shifted
        padded = expanded
    return padded


def fill_holes(mask: np.ndarray) -> np.ndarray:
    h, w = mask.shape
    visited = np.zeros_like(mask, dtype=bool)
    queue: deque[tuple[int, int]] = deque()

    def enqueue(y: int, x: int) -> None:
        if 0 <= y < h and 0 <= x < w and not visited[y, x] and not mask[y, x]:
            visited[y, x] = True
            queue.append((y, x))

    for x in range(w):
        enqueue(0, x)
        enqueue(h - 1, x)
    for y in range(h):
        enqueue(y, 0)
        enqueue(y, w - 1)

    while queue:
        y, x = queue.popleft()
        for dy in (-1, 0, 1):
            for dx in (-1, 0, 1):
                if dx == 0 and dy == 0:
                    continue
                enqueue(y + dy, x + dx)

    background = visited
    return ~background


def largest_component(mask: np.ndarray) -> np.ndarray:
    h, w = mask.shape
    visited = np.zeros_like(mask, dtype=bool)
    best_component: list[tuple[int, int]] = []

    for y in range(h):
        for x in range(w):
            if not mask[y, x] or visited[y, x]:
                continue
            queue: deque[tuple[int, int]] = deque()
            queue.append((y, x))
            visited[y, x] = True
            component: list[tuple[int, int]] = [(y, x)]

            while queue:
                cy, cx = queue.popleft()
                for dy in (-1, 0, 1):
                    for dx in (-1, 0, 1):
                        if dx == 0 and dy == 0:
                            continue
                        ny = cy + dy
                        nx = cx + dx
                        if 0 <= ny < h and 0 <= nx < w and mask[ny, nx] and not visited[ny, nx]:
                            visited[ny, nx] = True
                            queue.append((ny, nx))
                            component.append((ny, nx))

            if len(component) > len(best_component):
                best_component = component

    if not best_component:
        return mask

    cleaned = np.zeros_like(mask, dtype=bool)
    ys, xs = zip(*best_component)
    cleaned[np.array(ys), np.array(xs)] = True
    return cleaned


def erode_mask(mask: np.ndarray, radius: int) -> np.ndarray:
    if radius <= 0:
        return mask
    kernel = max(1, radius * 2 + 1)
    mask_img = Image.fromarray((mask.astype(np.uint8) * 255), mode="L")
    mask_img = mask_img.filter(ImageFilter.MinFilter(kernel))
    return np.asarray(mask_img) > 0


def find_bbox(mask: np.ndarray) -> tuple[int, int, int, int]:
    ys, xs = np.where(mask)
    if xs.size == 0 or ys.size == 0:
        raise ValueError("empty mask")
    return xs.min(), ys.min(), xs.max(), ys.max()


def estimate_knob_radius(gray: np.ndarray) -> int:
    h, w = gray.shape
    cx = w / 2.0
    cy = h / 2.0
    min_dim = min(w, h)
    r_min = int(min_dim * 0.18)
    r_max = int(min_dim * 0.45)
    angles = np.linspace(0, 2 * math.pi, 360, endpoint=False)
    cos_a = np.cos(angles)
    sin_a = np.sin(angles)

    ring_means = []
    for r in range(r_min, r_max + 2):
        xs = np.rint(cx + r * cos_a).astype(int)
        ys = np.rint(cy + r * sin_a).astype(int)
        xs = np.clip(xs, 0, w - 1)
        ys = np.clip(ys, 0, h - 1)
        ring_means.append(float(gray[ys, xs].mean()))

    scores = []
    for idx in range(1, len(ring_means) - 1):
        score = abs(ring_means[idx + 1] - ring_means[idx - 1])
        scores.append(score)

    best_idx = int(np.argmax(scores)) + 1
    return r_min + best_idx


def blur_alpha(alpha: np.ndarray, radius: float) -> np.ndarray:
    if radius <= 0:
        return alpha
    alpha_img = Image.fromarray((np.clip(alpha, 0.0, 1.0) * 255).astype(np.uint8), mode="L")
    alpha_img = alpha_img.filter(ImageFilter.GaussianBlur(radius))
    return np.asarray(alpha_img, dtype=np.float32) / 255.0


def erode_alpha(alpha: np.ndarray, radius: int) -> np.ndarray:
    if radius <= 0:
        return alpha
    kernel = max(1, radius * 2 + 1)
    alpha_img = Image.fromarray((np.clip(alpha, 0.0, 1.0) * 255).astype(np.uint8), mode="L")
    alpha_img = alpha_img.filter(ImageFilter.MinFilter(kernel))
    return np.asarray(alpha_img, dtype=np.float32) / 255.0


def compute_soft_alpha(arr: np.ndarray,
                       bg_color: np.ndarray,
                       threshold: float,
                       softness: float,
                       feather: float) -> tuple[np.ndarray, np.ndarray]:
    diff = np.sqrt(((arr - bg_color) ** 2).sum(axis=2))
    softness = max(softness, 1e-3)
    alpha = np.clip((diff - threshold) / softness, 0.0, 1.0)
    mask = alpha > 0.05
    mask = dilate(mask, iterations=1)
    mask = largest_component(mask)
    mask = fill_holes(mask)
    alpha = alpha * mask.astype(np.float32)
    alpha = blur_alpha(alpha, feather)
    return alpha, mask


def decontaminate_rgb(arr: np.ndarray, alpha: np.ndarray, bg_color: np.ndarray) -> np.ndarray:
    alpha_safe = np.clip(alpha, 1.0 / 255.0, 1.0)
    bg = bg_color.reshape(1, 1, 3)
    fg = (arr - bg * (1.0 - alpha_safe[..., None])) / alpha_safe[..., None]
    return np.clip(fg, 0.0, 255.0)


def rgba_from_rgb_alpha(rgb: np.ndarray, alpha: np.ndarray) -> Image.Image:
    rgba = np.zeros((rgb.shape[0], rgb.shape[1], 4), dtype=np.uint8)
    rgba[:, :, :3] = rgb.astype(np.uint8)
    rgba[:, :, 3] = (np.clip(alpha, 0.0, 1.0) * 255).astype(np.uint8)
    return Image.fromarray(rgba, mode="RGBA")


def clear_rgb_low_alpha(image: Image.Image, threshold: float) -> Image.Image:
    if threshold <= 0:
        return image
    arr = np.asarray(image).copy()
    alpha = arr[:, :, 3].astype(np.float32) / 255.0
    mask = alpha <= threshold
    arr[mask, :3] = 0
    return Image.fromarray(arr, mode="RGBA")


def pad_to_square(image: Image.Image) -> tuple[Image.Image, tuple[int, int]]:
    w, h = image.size
    size = max(w, h)
    offset_x = (size - w) // 2
    offset_y = (size - h) // 2
    canvas = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    canvas.paste(image, (offset_x, offset_y), image)
    return canvas, (offset_x, offset_y)


def offset_alpha(alpha: np.ndarray, dx: int, dy: int) -> np.ndarray:
    h, w = alpha.shape
    out = np.zeros_like(alpha)
    x0 = max(0, dx)
    y0 = max(0, dy)
    x1 = min(w, w + dx)
    y1 = min(h, h + dy)
    src_x0 = max(0, -dx)
    src_y0 = max(0, -dy)
    src_x1 = src_x0 + (x1 - x0)
    src_y1 = src_y0 + (y1 - y0)
    if x1 > x0 and y1 > y0:
        out[y0:y1, x0:x1] = alpha[src_y0:src_y1, src_x0:src_x1]
    return out


def build_highlight_layer(rgb: np.ndarray,
                          knob_mask: np.ndarray,
                          threshold: float,
                          chroma_limit: float,
                          strength: float,
                          blur_radius: float) -> Image.Image:
    rgb_norm = rgb / 255.0
    maxc = rgb_norm.max(axis=2)
    minc = rgb_norm.min(axis=2)
    chroma = maxc - minc
    lum = 0.2126 * rgb_norm[:, :, 0] + 0.7152 * rgb_norm[:, :, 1] + 0.0722 * rgb_norm[:, :, 2]

    highlight = np.clip((lum - threshold) / max(1e-3, 1.0 - threshold), 0.0, 1.0)
    chroma_mask = np.clip(1.0 - (chroma / max(1e-3, chroma_limit)), 0.0, 1.0)
    highlight = highlight * chroma_mask * knob_mask.astype(np.float32)
    highlight = blur_alpha(highlight, blur_radius)
    highlight = np.clip(highlight * strength, 0.0, 1.0)

    highlight_rgb = np.full((*highlight.shape, 3), 255, dtype=np.uint8)
    return rgba_from_rgb_alpha(highlight_rgb, highlight)


def build_shadow_layer(knob_mask: np.ndarray,
                       blur_radius: float,
                       offset: int,
                       strength: float) -> Image.Image:
    shadow = blur_alpha(knob_mask.astype(np.float32), blur_radius)
    shadow = offset_alpha(shadow, offset, offset)
    shadow = np.clip(shadow * strength, 0.0, 1.0)
    shadow_rgb = np.zeros((*shadow.shape, 3), dtype=np.uint8)
    return rgba_from_rgb_alpha(shadow_rgb, shadow)


def build_indicator_layer(size: int,
                          center: tuple[float, float],
                          radius: float,
                          width_px: float,
                          length_ratio: float,
                          color: str) -> Image.Image:
    indicator = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(indicator)
    cx, cy = center
    length = radius * length_ratio
    start = (cx, cy - length * 0.1)
    end = (cx, cy - length)
    line_width = max(1, int(round(width_px)))
    rgb = parse_hex_color(color)
    draw.line([start, end], fill=rgb + (255,), width=line_width)
    indicator = indicator.filter(ImageFilter.GaussianBlur(radius=max(0.0, width_px * 0.15)))
    return indicator


def extract_layers(path: Path,
                   output_dir: Path,
                   size: int,
                   bg_threshold: float,
                   bg_softness: float,
                   alpha_feather: float,
                   plate_alpha_cut: float,
                   plate_erode: int,
                   plate_shadow_blur: float,
                   plate_shadow_offset: int,
                   plate_shadow_strength: float,
                   plate_edge_width: int,
                   plate_edge_darken: float,
                   alpha_clip: float,
                   pad_ratio: float,
                   knob_edge_offset: float,
                   knob_feather: float,
                   knob_cutout_feather: float,
                   shadow_blur: float,
                   shadow_offset: int,
                   shadow_strength: float,
                   highlight_threshold: float,
                   highlight_chroma: float,
                   highlight_strength: float,
                   highlight_blur: float,
                   indicator_width: float,
                   indicator_length: float,
                   indicator_color: str) -> dict:
    image = Image.open(path).convert("RGBA")
    arr = np.asarray(image)[:, :, :3].astype(np.float32)
    bg_color = estimate_background_color(arr)
    alpha, mask = compute_soft_alpha(arr, bg_color, bg_threshold, bg_softness, alpha_feather)

    x0, y0, x1, y1 = find_bbox(mask)
    w = x1 - x0 + 1
    h = y1 - y0 + 1
    pad = int(max(w, h) * pad_ratio)
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(image.width - 1, x1 + pad)
    y1 = min(image.height - 1, y1 + pad)

    crop_box = (x0, y0, x1 + 1, y1 + 1)
    image_crop = image.crop(crop_box)
    alpha_crop = alpha[y0 : y1 + 1, x0 : x1 + 1]
    rgb_crop = arr[y0 : y1 + 1, x0 : x1 + 1]
    fg_crop = decontaminate_rgb(rgb_crop, alpha_crop, bg_color)

    plate_alpha = np.clip((alpha_crop - plate_alpha_cut) / max(1e-3, 1.0 - plate_alpha_cut), 0.0, 1.0)
    plate_alpha = erode_alpha(plate_alpha, plate_erode)
    plate = rgba_from_rgb_alpha(fg_crop, plate_alpha)
    plate_square, offset = pad_to_square(plate)

    gray = np.asarray(image_crop.convert("L"))
    knob_radius = estimate_knob_radius(gray) - knob_edge_offset
    knob_radius = max(knob_radius, 10)

    square_size = plate_square.size[0]
    cx = (image_crop.width / 2.0) + offset[0]
    cy = (image_crop.height / 2.0) + offset[1]

    yy, xx = np.ogrid[:square_size, :square_size]
    dist2 = (xx - cx) ** 2 + (yy - cy) ** 2
    knob_mask = dist2 <= (knob_radius ** 2)

    knob_canvas = np.zeros((square_size, square_size, 4), dtype=np.uint8)
    ox, oy = offset
    knob_canvas[oy : oy + fg_crop.shape[0], ox : ox + fg_crop.shape[1], :3] = fg_crop.astype(np.uint8)
    knob_alpha = blur_alpha(knob_mask.astype(np.float32), knob_feather)
    knob_canvas[:, :, 3] = (knob_alpha * 255).astype(np.uint8)
    knob = Image.fromarray(knob_canvas, mode="RGBA")

    plate_arr = np.asarray(plate_square).copy()
    plate_alpha = plate_arr[:, :, 3].astype(np.float32) / 255.0
    cutout_alpha = blur_alpha(knob_mask.astype(np.float32), knob_cutout_feather)
    plate_alpha = plate_alpha * (1.0 - cutout_alpha)
    plate_arr[:, :, 3] = (np.clip(plate_alpha, 0.0, 1.0) * 255).astype(np.uint8)
    plate_square = Image.fromarray(plate_arr, mode="RGBA")

    plate_mask = plate_alpha > 0.05
    plate_mask = fill_holes(plate_mask)
    if plate_edge_width > 0 and plate_edge_darken > 0.0:
        plate_arr = np.asarray(plate_square).copy()
        inner_mask = erode_mask(plate_mask, plate_edge_width)
        edge_mask = plate_mask & ~inner_mask
        if np.any(edge_mask):
            edge_rgb = plate_arr[:, :, :3].astype(np.float32)
            edge_rgb[edge_mask] *= max(0.0, 1.0 - plate_edge_darken)
            plate_arr[:, :, :3] = edge_rgb.astype(np.uint8)
            plate_square = Image.fromarray(plate_arr, mode="RGBA")
    plate_shadow = Image.new("RGBA", plate_square.size, (0, 0, 0, 0))
    if plate_shadow_strength > 0.0:
        plate_shadow = build_shadow_layer(plate_mask, plate_shadow_blur, plate_shadow_offset, plate_shadow_strength)

    shadow = build_shadow_layer(knob_mask, shadow_blur, shadow_offset, shadow_strength)
    highlight = build_highlight_layer(
        knob_canvas[:, :, :3].astype(np.float32),
        knob_mask,
        highlight_threshold,
        highlight_chroma,
        highlight_strength,
        highlight_blur,
    )
    indicator = build_indicator_layer(
        square_size,
        (cx, cy),
        knob_radius,
        indicator_width,
        indicator_length,
        indicator_color,
    )

    plate_square = clear_rgb_low_alpha(plate_square, alpha_clip)
    plate_shadow = clear_rgb_low_alpha(plate_shadow, alpha_clip)
    knob = clear_rgb_low_alpha(knob, alpha_clip)
    highlight = clear_rgb_low_alpha(highlight, alpha_clip)
    shadow = clear_rgb_low_alpha(shadow, alpha_clip)
    indicator = clear_rgb_low_alpha(indicator, alpha_clip)

    if size > 0 and size != plate_square.size[0]:
        plate_square = plate_square.resize((size, size), Image.LANCZOS)
        plate_shadow = plate_shadow.resize((size, size), Image.LANCZOS)
        knob = knob.resize((size, size), Image.LANCZOS)
        highlight = highlight.resize((size, size), Image.LANCZOS)
        shadow = shadow.resize((size, size), Image.LANCZOS)
        indicator = indicator.resize((size, size), Image.LANCZOS)

    preview = Image.new("RGBA", plate_square.size, (0, 0, 0, 0))
    preview.paste(plate_shadow, (0, 0), plate_shadow)
    preview.paste(plate_square, (0, 0), plate_square)
    preview.paste(shadow, (0, 0), shadow)
    preview.paste(knob, (0, 0), knob)
    preview.paste(highlight, (0, 0), highlight)
    preview.paste(indicator, (0, 0), indicator)

    return {
        "plate": plate_square,
        "plate_shadow": plate_shadow,
        "knob": knob,
        "highlight": highlight,
        "shadow": shadow,
        "indicator": indicator,
        "preview": preview,
        "meta": {
            "source": str(path),
            "crop_box": [int(x0), int(y0), int(x1), int(y1)],
            "bg_color": [float(v) for v in bg_color],
            "knob_radius": float(knob_radius),
            "output_size": size,
        },
    }


def main() -> int:
    parser = argparse.ArgumentParser(description="Extract Line 6 knob layers.")
    parser.add_argument("--input-dir", type=Path, default=DEFAULT_INPUT_DIR)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--size", type=int, default=512)
    parser.add_argument("--bg-threshold", type=float, default=18.0)
    parser.add_argument("--bg-softness", type=float, default=12.0)
    parser.add_argument("--alpha-feather", type=float, default=1.2)
    parser.add_argument("--pad-ratio", type=float, default=0.02)
    parser.add_argument("--plate-alpha-cut", type=float, default=0.12)
    parser.add_argument("--plate-erode", type=int, default=0)
    parser.add_argument("--plate-shadow-blur", type=float, default=12.0)
    parser.add_argument("--plate-shadow-offset", type=int, default=6)
    parser.add_argument("--plate-shadow-strength", type=float, default=0.0)
    parser.add_argument("--plate-edge-width", type=int, default=0)
    parser.add_argument("--plate-edge-darken", type=float, default=0.0)
    parser.add_argument("--alpha-clip", type=float, default=0.0)
    parser.add_argument("--knob-edge-offset", type=float, default=4.0)
    parser.add_argument("--knob-feather", type=float, default=1.2)
    parser.add_argument("--knob-cutout-feather", type=float, default=1.6)
    parser.add_argument("--shadow-blur", type=float, default=8.0)
    parser.add_argument("--shadow-offset", type=int, default=6)
    parser.add_argument("--shadow-strength", type=float, default=0.35)
    parser.add_argument("--highlight-threshold", type=float, default=0.78)
    parser.add_argument("--highlight-chroma", type=float, default=0.18)
    parser.add_argument("--highlight-strength", type=float, default=0.4)
    parser.add_argument("--highlight-blur", type=float, default=1.6)
    parser.add_argument("--indicator-width", type=float, default=6.0)
    parser.add_argument("--indicator-length", type=float, default=0.62)
    parser.add_argument("--indicator-color", type=str, default="#caa254")
    parser.add_argument("--prefix", type=str, default="line6_")
    parser.add_argument("--single-name", type=str, default="")
    args = parser.parse_args()

    input_dir = args.input_dir.expanduser()
    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    manifest = []
    inputs = sorted(input_dir.glob("*.png"))
    if args.single_name and len(inputs) != 1:
        raise SystemExit("--single-name requires exactly one input PNG")

    for path in inputs:
        slug = args.single_name.strip() if args.single_name else slugify(path.stem, args.prefix)
        variant_dir = output_dir / slug
        variant_dir.mkdir(parents=True, exist_ok=True)

        result = extract_layers(
            path,
            variant_dir,
            args.size,
            args.bg_threshold,
            args.bg_softness,
            args.alpha_feather,
            args.plate_alpha_cut,
            args.plate_erode,
            args.plate_shadow_blur,
            args.plate_shadow_offset,
            args.plate_shadow_strength,
            args.plate_edge_width,
            args.plate_edge_darken,
            args.alpha_clip,
            args.pad_ratio,
            args.knob_edge_offset,
            args.knob_feather,
            args.knob_cutout_feather,
            args.shadow_blur,
            args.shadow_offset,
            args.shadow_strength,
            args.highlight_threshold,
            args.highlight_chroma,
            args.highlight_strength,
            args.highlight_blur,
            args.indicator_width,
            args.indicator_length,
            args.indicator_color,
        )
        plate_name = f"{slug}_plate.png"
        plate_shadow_name = f"{slug}_plate_shadow.png"
        knob_name = f"{slug}_knob.png"
        highlight_name = f"{slug}_highlight.png"
        shadow_name = f"{slug}_shadow.png"
        indicator_name = f"{slug}_indicator.png"
        preview_name = f"{slug}_preview.png"

        result["plate"].save(variant_dir / plate_name)
        result["plate_shadow"].save(variant_dir / plate_shadow_name)
        result["knob"].save(variant_dir / knob_name)
        result["highlight"].save(variant_dir / highlight_name)
        result["shadow"].save(variant_dir / shadow_name)
        result["indicator"].save(variant_dir / indicator_name)
        result["preview"].save(variant_dir / preview_name)

        meta_path = variant_dir / "meta.json"
        meta_path.write_text(json.dumps(result["meta"], indent=2), encoding="utf-8")

        manifest.append(
            {
                "name": slug,
                "plate": str((variant_dir / plate_name).as_posix()),
                "plate_shadow": str((variant_dir / plate_shadow_name).as_posix()),
                "knob": str((variant_dir / knob_name).as_posix()),
                "highlight": str((variant_dir / highlight_name).as_posix()),
                "shadow": str((variant_dir / shadow_name).as_posix()),
                "indicator": str((variant_dir / indicator_name).as_posix()),
                "preview": str((variant_dir / preview_name).as_posix()),
                "meta": str(meta_path.as_posix()),
            }
        )

    manifest_path = output_dir / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")

    print(f"Extracted {len(manifest)} knob variants to {output_dir}")
    print(f"Manifest: {manifest_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
