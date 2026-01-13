#!/usr/bin/env python3
"""
Generate Archive Instruments knob renders with the OpenAI image API.
Outputs raw PNGs for downstream layer extraction.
"""
from __future__ import annotations

import argparse
import base64
import json
import urllib.request
from datetime import datetime
from pathlib import Path

from openai import OpenAI


DEFAULT_PROMPT = (
    "Photorealistic rotary knob UI asset, Archive Instruments LOXLOOP style. "
    "Orthographic top-down view, perfectly centered, no perspective tilt. "
    "Vintage industrial materials: worn brass mounting plate with engraved tick marks "
    "and 0-10 numerals, knurled knob edge, radial brushed metal cap, subtle patina, "
    "micro-scratches, edge wear. Soft studio lighting from top-left, minimal shadow, "
    "no vignette. Pure white background. High realism, consistent lighting."
)


def decode_image(item) -> bytes:
    if hasattr(item, "b64_json") and item.b64_json:
        return base64.b64decode(item.b64_json)
    if isinstance(item, dict) and item.get("b64_json"):
        return base64.b64decode(item["b64_json"])
    if hasattr(item, "url") and item.url:
        with urllib.request.urlopen(item.url) as resp:
            return resp.read()
    if isinstance(item, dict) and item.get("url"):
        with urllib.request.urlopen(item["url"]) as resp:
            return resp.read()
    raise RuntimeError("No image data found in response.")


def main() -> int:
    parser = argparse.ArgumentParser(description="Generate Archive Instruments knob renders.")
    parser.add_argument("--output-dir", type=Path, default=Path("assets/ui/archive/raw"))
    parser.add_argument("--name", type=str, default="archive_brass_precision")
    parser.add_argument("--size", type=int, default=1024)
    parser.add_argument("--model", type=str, default="gpt-image-1")
    parser.add_argument("--count", type=int, default=1)
    parser.add_argument("--prompt", type=str, default=DEFAULT_PROMPT)
    args = parser.parse_args()

    output_dir = args.output_dir
    output_dir.mkdir(parents=True, exist_ok=True)

    client = OpenAI()
    params = {
        "model": args.model,
        "prompt": args.prompt,
        "size": f"{args.size}x{args.size}",
        "n": args.count,
    }

    try:
        result = client.images.generate(**params, background="white")
    except Exception:
        result = client.images.generate(**params)

    outputs = []
    for idx, item in enumerate(result.data):
        image_bytes = decode_image(item)
        suffix = f"_{idx + 1}" if args.count > 1 else ""
        filename = f"{args.name}{suffix}.png"
        path = output_dir / filename
        path.write_bytes(image_bytes)
        outputs.append(path.as_posix())

    meta = {
        "name": args.name,
        "model": args.model,
        "size": args.size,
        "count": args.count,
        "prompt": args.prompt,
        "outputs": outputs,
        "timestamp": datetime.utcnow().isoformat() + "Z",
    }
    meta_path = output_dir / f"{args.name}_meta.json"
    meta_path.write_text(json.dumps(meta, indent=2), encoding="utf-8")

    print(f"Generated {len(outputs)} image(s) in {output_dir}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
