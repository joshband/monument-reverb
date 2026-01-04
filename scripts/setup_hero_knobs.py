#!/usr/bin/env python3
"""
Setup hero knobs workspace for materialize pipeline
Copies selected knob series and prepares them for PBR texture generation
"""
import os
import shutil
from pathlib import Path

# Define workspace paths
KNOBS_SOURCE = Path.home() / 'Desktop' / 'knobs'
HERO_WORKSPACE = Path.home() / 'Documents' / '3_Development' / 'Repos' / 'materialize' / 'input' / 'hero_knobs'
OUTPUT_BASE = Path.home() / 'Documents' / '3_Development' / 'Repos' / 'materialize' / 'dist' / 'hero_knobs'

# Selected series (by UUID)
SELECTED_SERIES = [
    '45087b68-2047-4ab1-a014-42daee1a2162',  # Series 1
    '996dd52e-4f61-4a39-8c84-1633a8a92707',  # Series 2
    '855f2c8b-55f8-48c8-9a4e-be48d5e15d06',  # Series 3
]

def setup_workspace():
    """Create workspace directory structure"""
    print("🔧 Setting up hero knobs workspace...")
    print("=" * 80)

    # Create workspace
    HERO_WORKSPACE.mkdir(parents=True, exist_ok=True)
    print(f"✅ Workspace: {HERO_WORKSPACE}")

    # Create output directory
    OUTPUT_BASE.mkdir(parents=True, exist_ok=True)
    print(f"✅ Output: {OUTPUT_BASE}")

    return True

def copy_series(series_uuid, series_num):
    """Copy all variations of a series to workspace"""
    pattern = f"stone_rotary_knob_glowing_led_center_decorative_isolated_on_p_{series_uuid}_*.png"

    # Find all files matching this series
    files = list(KNOBS_SOURCE.glob(pattern))

    if not files:
        print(f"⚠️  No files found for series {series_num} (UUID: {series_uuid})")
        return False

    print(f"\n📦 Series {series_num}: {series_uuid}")
    print(f"   Found {len(files)} variations")

    # Copy each file
    for src_file in sorted(files):
        # Create shorter destination name
        variation = src_file.name.split('_')[-1]  # Get _0.png, _1.png, etc.
        dst_name = f"series_{series_num}_{variation}"
        dst_path = HERO_WORKSPACE / dst_name

        shutil.copy2(src_file, dst_path)
        print(f"   ✅ {dst_name}")

    return True

def create_processing_manifest():
    """Create a manifest file listing all images to process"""
    manifest_path = HERO_WORKSPACE / '_manifest.txt'

    with open(manifest_path, 'w') as f:
        f.write("Hero Knobs Processing Manifest\n")
        f.write("=" * 80 + "\n\n")
        f.write("Selected Series:\n\n")

        for i, uuid in enumerate(SELECTED_SERIES, 1):
            f.write(f"Series {i}: {uuid}\n")
            f.write(f"  Files: series_{i}_0.png through series_{i}_3.png\n\n")

        f.write("\nNext Steps:\n")
        f.write("1. Review images and select best variation from each series\n")
        f.write("2. Run masking script to remove backgrounds and create alpha channels\n")
        f.write("3. Process through materialize pipeline:\n")
        f.write("   cd ~/Documents/3_Development/Repos/materialize\n")
        f.write("   python -m materialize --in input/hero_knobs --out dist/hero_knobs\n")
        f.write("4. Review generated PBR textures (albedo, normal, roughness, height)\n")

    print(f"\n📄 Manifest created: {manifest_path}")

def main():
    print("\n🎯 HERO KNOBS SETUP")
    print("=" * 80 + "\n")

    # Setup workspace
    if not setup_workspace():
        print("❌ Failed to create workspace")
        return

    # Copy all selected series
    print("\n📋 Copying selected series...")
    for i, uuid in enumerate(SELECTED_SERIES, 1):
        copy_series(uuid, i)

    # Create manifest
    create_processing_manifest()

    # Summary
    print("\n" + "=" * 80)
    print("✅ SETUP COMPLETE")
    print("=" * 80)
    print(f"\n📁 Workspace: {HERO_WORKSPACE}")
    print(f"📁 Output: {OUTPUT_BASE}")
    print(f"\n💡 Next: Review images and run masking script")
    print(f"   open {HERO_WORKSPACE}")

if __name__ == '__main__':
    main()
