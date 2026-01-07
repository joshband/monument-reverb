#!/usr/bin/env python3
"""Extract binary assets from git patch file."""

import re
import subprocess
import sys
from pathlib import Path

def extract_binary_files(patch_file, output_dir, file_pattern):
    """Extract binary files matching pattern from git patch."""

    patch_content = Path(patch_file).read_text(encoding='utf-8', errors='ignore')

    # Find all binary file sections
    # Pattern: diff --git a/<file> b/<file> ... GIT binary patch ...
    pattern = r'diff --git a/(' + file_pattern + r') b/\1.*?(?=diff --git|$)'

    matches = re.finditer(pattern, patch_content, re.DOTALL)

    extracted_files = []

    for match in matches:
        file_section = match.group(0)
        file_path = match.group(1)

        # Create a temporary patch file for this single file
        temp_patch = output_dir / 'temp_single.patch'
        temp_patch.write_text(file_section)

        # Use git apply to extract the file
        try:
            result = subprocess.run(
                ['git', 'apply', str(temp_patch)],
                cwd=str(output_dir.parent),
                capture_output=True,
                text=True
            )

            if result.returncode == 0:
                extracted_files.append(file_path)
                print(f"✓ Extracted: {file_path}")
            else:
                print(f"✗ Failed: {file_path}")
                print(f"  Error: {result.stderr[:200]}")
        except Exception as e:
            print(f"✗ Exception for {file_path}: {e}")

        # Clean up temp file
        if temp_patch.exists():
            temp_patch.unlink()

    return extracted_files

def main():
    repo_root = Path(__file__).parent
    patch_file = repo_root / 'patch.txt'
    output_dir = repo_root / 'assets' / 'knob_geode'

    # Ensure output directory exists
    output_dir.mkdir(parents=True, exist_ok=True)

    print("Extracting knob_geode assets from patch.txt...")
    print(f"Output directory: {output_dir}")
    print()

    # Extract PNG files
    extracted = extract_binary_files(patch_file, output_dir, r'assets/knob_geode/\w+\.png')

    # Extract manifest.json
    manifest_extracted = extract_binary_files(patch_file, output_dir, r'assets/knob_geode/manifest\.json')
    extracted.extend(manifest_extracted)

    print()
    print(f"Total files extracted: {len(extracted)}")

    if len(extracted) >= 11:  # 10 PNGs + manifest.json
        print("✓ All expected files extracted successfully!")
        return 0
    else:
        print(f"⚠ Expected 11 files, but only extracted {len(extracted)}")
        return 1

if __name__ == '__main__':
    sys.exit(main())
