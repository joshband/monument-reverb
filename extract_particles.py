#!/usr/bin/env python3
"""
Extract particle system files from patch.txt
Creates Source/Particles/ directory with all source files and presets
"""

import os
import re
import sys

def extract_particle_files(patch_path, output_base):
    """Extract all particle files from the patch"""

    with open(patch_path, 'r', encoding='utf-8', errors='ignore') as f:
        patch_content = f.read()

    # Find all particle file sections
    particle_pattern = r'diff --git a/(Source/Particles/[^\s]+) b/\1\n.*?\n\+\+\+ b/\1\n(.*?)(?=\ndiff --git|\Z)'

    matches = re.finditer(particle_pattern, patch_content, re.DOTALL)

    extracted = []

    for match in matches:
        file_path = match.group(1)
        diff_content = match.group(2)

        # Parse the diff content to extract the actual file
        lines = diff_content.split('\n')
        file_lines = []

        for line in lines:
            if line.startswith('+') and not line.startswith('+++'):
                # Remove the '+' prefix
                file_lines.append(line[1:])
            elif not line.startswith('-') and not line.startswith('@@') and not line.startswith('\\'):
                # Keep context lines (no prefix)
                if line and not line.startswith('index ') and not line.startswith('new file'):
                    file_lines.append(line)

        # Create output path
        output_path = os.path.join(output_base, file_path)
        os.makedirs(os.path.dirname(output_path), exist_ok=True)

        # Write file
        with open(output_path, 'w', encoding='utf-8') as out:
            out.write('\n'.join(file_lines))

        file_size = os.path.getsize(output_path)
        extracted.append((file_path, file_size))
        print(f"✓ Extracted: {file_path} ({file_size:,} bytes)")

    return extracted

if __name__ == '__main__':
    patch_path = 'patch.txt'
    output_base = '.'

    if not os.path.exists(patch_path):
        print(f"Error: {patch_path} not found")
        sys.exit(1)

    print("Extracting particle system files from patch...")
    print()

    extracted = extract_particle_files(patch_path, output_base)

    print()
    print(f"✓ Successfully extracted {len(extracted)} particle files")
    print()

    # Organize by type
    source_files = [f for f in extracted if f[0].endswith(('.cpp', '.h'))]
    docs = [f for f in extracted if f[0].endswith('.md')]
    presets = [f for f in extracted if f[0].endswith('.json')]

    print("Summary:")
    print(f"  - Source files: {len(source_files)}")
    print(f"  - Documentation: {len(docs)}")
    print(f"  - Presets: {len(presets)}")
    print()

    total_size = sum(f[1] for f in extracted)
    print(f"Total size: {total_size:,} bytes ({total_size / 1024:.1f} KB)")
