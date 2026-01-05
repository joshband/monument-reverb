#!/usr/bin/env python3
"""
Validate a photorealistic RGBA component pack.

Checks:
1. JSON structure (schema-valid)
2. Image integrity (same size, alpha present)
3. Semantic rules (indicator layer exists, states reference real layers)

Usage:
    python validate_component_pack.py <component-pack-folder>
    
Requirements:
    pip install pillow jsonschema
"""

import json
import sys
from pathlib import Path

try:
    from PIL import Image
    from jsonschema import Draft202012Validator
except ImportError:
    print("Missing dependencies. Install with:")
    print("  pip install pillow jsonschema")
    sys.exit(1)

SCHEMA_PATH = Path(__file__).parent.parent / "schemas" / "component.manifest.schema.json"


def fail(msg: str):
    print(f"❌ VALIDATION FAILED: {msg}")
    sys.exit(1)


def warn(msg: str):
    print(f"⚠️  WARNING: {msg}")


def load_schema():
    if not SCHEMA_PATH.exists():
        fail(f"Schema not found: {SCHEMA_PATH}")
    with open(SCHEMA_PATH, "r") as f:
        return json.load(f)


def validate_schema(manifest: dict):
    """Validate manifest against JSON schema."""
    schema = load_schema()
    validator = Draft202012Validator(schema)
    errors = sorted(validator.iter_errors(manifest), key=lambda e: list(e.path))
    
    if errors:
        print("Schema validation errors:")
        for e in errors:
            path = " → ".join(str(p) for p in e.path) or "(root)"
            print(f"  [{path}]: {e.message}")
        fail("Schema validation failed")
    
    print("✓ Schema valid")


def validate_images(pack_dir: Path, manifest: dict):
    """Validate all layer images exist, have same size, and have alpha."""
    layers = manifest.get("layers", [])
    if not layers:
        fail("No layers defined in manifest")
    
    layer_files = []
    for layer in layers:
        layer_path = pack_dir / layer["file"]
        if not layer_path.exists():
            fail(f"Missing image file: {layer['file']}")
        layer_files.append((layer["id"], layer_path))
    
    # Check first image for reference dimensions
    first_id, first_path = layer_files[0]
    first_img = Image.open(first_path)
    ref_width, ref_height = first_img.size
    
    if first_img.mode not in ("RGBA", "LA"):
        fail(f"Layer '{first_id}' has no alpha channel (mode: {first_img.mode})")
    
    print(f"✓ Reference size: {ref_width}x{ref_height}")
    
    # Validate all images
    for layer_id, layer_path in layer_files:
        img = Image.open(layer_path)
        
        # Check dimensions match
        if img.size != (ref_width, ref_height):
            fail(f"Layer '{layer_id}' size mismatch: {img.size} (expected {ref_width}x{ref_height})")
        
        # Check alpha channel
        if "A" not in img.mode:
            fail(f"Layer '{layer_id}' has no alpha channel (mode: {img.mode})")
    
    print(f"✓ All {len(layer_files)} images valid")


def validate_semantics(manifest: dict):
    """Validate semantic rules (indicator for knobs, state references)."""
    component = manifest.get("component", {})
    layers = manifest.get("layers", [])
    state_logic = manifest.get("stateLogic", {})
    
    layer_ids = {layer["id"] for layer in layers}
    states = component.get("behavior", {}).get("states", [])
    comp_type = component.get("type", "")
    
    # Knobs must have indicator layer
    if comp_type == "knob.rotary":
        indicator_layers = [l for l in layers if l.get("role", "").startswith("indicator")]
        if not indicator_layers:
            fail("Knob component requires an indicator layer (role: 'indicator.*')")
        print("✓ Indicator layer present")
    
    # Validate state logic references
    for state_name, state_def in state_logic.items():
        if state_name not in states:
            warn(f"stateLogic references undefined state: '{state_name}'")
        
        # Check visible layer references
        for layer_id in state_def.get("visible", []):
            if layer_id not in layer_ids:
                fail(f"State '{state_name}' references unknown layer '{layer_id}'")
        
        # Check opacity override references
        for layer_id in state_def.get("opacityOverrides", {}).keys():
            if layer_id not in layer_ids:
                fail(f"State '{state_name}' opacity override references unknown layer '{layer_id}'")
    
    print(f"✓ State logic valid ({len(state_logic)} states)")


def main():
    if len(sys.argv) != 2:
        print("Usage: validate_component_pack.py <component-pack-folder>")
        print("\nValidates a photorealistic RGBA component pack:")
        print("  - JSON schema compliance")
        print("  - Image dimensions match")
        print("  - Alpha channels present")
        print("  - Layer and state references valid")
        sys.exit(1)
    
    pack_dir = Path(sys.argv[1])
    
    if not pack_dir.is_dir():
        fail(f"Not a directory: {pack_dir}")
    
    manifest_path = pack_dir / "component.manifest.json"
    
    if not manifest_path.exists():
        fail(f"component.manifest.json not found in {pack_dir}")
    
    print(f"Validating: {pack_dir}")
    print("-" * 40)
    
    # Load manifest
    with open(manifest_path, "r") as f:
        try:
            manifest = json.load(f)
        except json.JSONDecodeError as e:
            fail(f"Invalid JSON: {e}")
    
    # Run validations
    validate_schema(manifest)
    validate_images(pack_dir, manifest)
    validate_semantics(manifest)
    
    print("-" * 40)
    print("✅ Component pack is valid")


if __name__ == "__main__":
    main()
