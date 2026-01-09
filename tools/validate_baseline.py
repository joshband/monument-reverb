#!/usr/bin/env python3
"""
Baseline Data Validation Script

Purpose: Validate integrity, completeness, and correctness of baseline test data.

Usage:
    python3 tools/validate_baseline.py <baseline_dir>
    python3 tools/validate_baseline.py test-results/baseline-ci

Exit codes:
    0 - All validations passed
    1 - Validation failures detected
    2 - Missing dependencies or invalid arguments
"""

import sys
import os
import json
import hashlib
from pathlib import Path
from typing import Dict, List, Tuple, Optional

# ANSI color codes
GREEN = '\033[92m'
RED = '\033[91m'
YELLOW = '\033[93m'
BLUE = '\033[94m'
RESET = '\033[0m'


class BaselineValidator:
    """Validates baseline test data directory structure and content."""

    EXPECTED_PRESETS = 37  # Monument has 37 factory presets
    REQUIRED_FILES = ['wet.wav', 'dry.wav', 'metadata.json', 'rt60_metrics.json', 'freq_metrics.json']
    OPTIONAL_FILES = ['frequency_response.png', 'rt60_analysis.log']

    def __init__(self, baseline_dir: str):
        self.baseline_dir = Path(baseline_dir)
        self.errors: List[str] = []
        self.warnings: List[str] = []
        self.info_messages: List[str] = []

    def validate(self) -> bool:
        """Run all validation checks. Returns True if all pass."""
        print(f"{BLUE}Validating baseline data: {self.baseline_dir}{RESET}\n")

        # Check directory exists
        if not self.baseline_dir.exists():
            self.error(f"Baseline directory does not exist: {self.baseline_dir}")
            return False

        if not self.baseline_dir.is_dir():
            self.error(f"Path is not a directory: {self.baseline_dir}")
            return False

        # Run validation checks
        self.check_preset_count()
        self.check_file_structure()
        self.check_metadata_validity()
        self.check_rt60_metrics()
        self.check_frequency_metrics()
        self.check_audio_files()
        self.check_data_consistency()

        # Print summary
        self.print_summary()

        return len(self.errors) == 0

    def check_preset_count(self):
        """Verify expected number of preset directories."""
        preset_dirs = [d for d in self.baseline_dir.iterdir()
                      if d.is_dir() and d.name.startswith('preset_')]

        found_count = len(preset_dirs)

        if found_count == 0:
            self.error("No preset directories found")
        elif found_count < self.EXPECTED_PRESETS:
            self.error(f"Missing presets: found {found_count}/{self.EXPECTED_PRESETS}")
        elif found_count > self.EXPECTED_PRESETS:
            self.warning(f"Extra presets: found {found_count}/{self.EXPECTED_PRESETS}")
        else:
            self.info(f"✓ Correct preset count: {found_count}")

    def check_file_structure(self):
        """Verify required files exist in each preset directory."""
        missing_files = {}

        for preset_idx in range(self.EXPECTED_PRESETS):
            preset_dir = self.baseline_dir / f"preset_{preset_idx:02d}"

            if not preset_dir.exists():
                self.error(f"Missing preset directory: preset_{preset_idx:02d}")
                continue

            for required_file in self.REQUIRED_FILES:
                file_path = preset_dir / required_file
                if not file_path.exists():
                    if preset_idx not in missing_files:
                        missing_files[preset_idx] = []
                    missing_files[preset_idx].append(required_file)

        if missing_files:
            for preset_idx, files in missing_files.items():
                self.error(f"Preset {preset_idx:02d} missing files: {', '.join(files)}")
        else:
            self.info(f"✓ All required files present")

    def check_metadata_validity(self):
        """Validate metadata.json structure and content."""
        invalid_metadata = []

        for preset_idx in range(self.EXPECTED_PRESETS):
            metadata_path = self.baseline_dir / f"preset_{preset_idx:02d}" / "metadata.json"

            if not metadata_path.exists():
                continue  # Already reported in check_file_structure

            try:
                with open(metadata_path, 'r') as f:
                    metadata = json.load(f)

                # Check critical fields (flexible to support both old and new schema)
                critical_fields = ['sample_rate', 'duration_seconds']
                missing = [f for f in critical_fields if f not in metadata]

                if missing:
                    invalid_metadata.append((preset_idx, f"missing critical fields: {', '.join(missing)}"))
                    continue

                # Validate sample rate
                if metadata['sample_rate'] not in [44100, 48000, 88200, 96000]:
                    self.warning(f"Preset {preset_idx:02d}: unusual sample rate {metadata['sample_rate']}Hz")

                # Optional validation for num_channels (if present)
                if 'num_channels' in metadata and metadata['num_channels'] not in [1, 2]:
                    invalid_metadata.append((preset_idx, f"invalid num_channels: {metadata['num_channels']}"))

                # Optional validation for test_type (if present)
                if 'test_type' in metadata and metadata['test_type'] not in ['impulse', 'sweep', 'noise', 'pink']:
                    invalid_metadata.append((preset_idx, f"invalid test_type: {metadata['test_type']}"))

            except json.JSONDecodeError as e:
                invalid_metadata.append((preset_idx, f"JSON parse error: {e}"))
            except Exception as e:
                invalid_metadata.append((preset_idx, f"error: {e}"))

        if invalid_metadata:
            for preset_idx, reason in invalid_metadata:
                self.error(f"Preset {preset_idx:02d} metadata invalid: {reason}")
        else:
            self.info(f"✓ All metadata valid")

    def check_rt60_metrics(self):
        """Validate RT60 metrics JSON."""
        invalid_rt60 = []
        rt60_range_violations = []

        for preset_idx in range(self.EXPECTED_PRESETS):
            rt60_path = self.baseline_dir / f"preset_{preset_idx:02d}" / "rt60_metrics.json"

            if not rt60_path.exists():
                continue

            try:
                with open(rt60_path, 'r') as f:
                    rt60 = json.load(f)

                # Check required fields
                if 'broadband' not in rt60 or 'rt60_seconds' not in rt60['broadband']:
                    invalid_rt60.append((preset_idx, "missing broadband.rt60_seconds"))
                    continue

                rt60_value = rt60['broadband']['rt60_seconds']

                # Validate RT60 range (typical reverbs: 0.1s - 60s)
                if not (0.1 <= rt60_value <= 60.0):
                    rt60_range_violations.append((preset_idx, rt60_value))

            except json.JSONDecodeError as e:
                invalid_rt60.append((preset_idx, f"JSON parse error: {e}"))
            except Exception as e:
                invalid_rt60.append((preset_idx, f"error: {e}"))

        if invalid_rt60:
            for preset_idx, reason in invalid_rt60:
                self.error(f"Preset {preset_idx:02d} RT60 metrics invalid: {reason}")

        if rt60_range_violations:
            for preset_idx, value in rt60_range_violations:
                self.warning(f"Preset {preset_idx:02d}: RT60 {value:.2f}s outside typical range (0.1-60s)")

        if not invalid_rt60:
            self.info(f"✓ All RT60 metrics valid")

    def check_frequency_metrics(self):
        """Validate frequency response metrics JSON."""
        invalid_freq = []

        for preset_idx in range(self.EXPECTED_PRESETS):
            freq_path = self.baseline_dir / f"preset_{preset_idx:02d}" / "freq_metrics.json"

            if not freq_path.exists():
                continue

            try:
                with open(freq_path, 'r') as f:
                    freq = json.load(f)

                # Check for either old schema (broadband/quality_rating) or new schema (overall/flatness_rating)
                has_old_schema = 'broadband' in freq and 'quality_rating' in freq
                has_new_schema = 'overall' in freq or 'octave_bands' in freq

                if not (has_old_schema or has_new_schema):
                    invalid_freq.append((preset_idx, "missing frequency data (no broadband/overall or octave_bands)"))
                    continue

                # Validate old schema if present
                if 'quality_rating' in freq:
                    valid_ratings = ['Excellent', 'Good', 'Fair', 'Colored']
                    if freq['quality_rating'] not in valid_ratings:
                        invalid_freq.append((preset_idx, f"invalid quality_rating: {freq['quality_rating']}"))

                # Check flatness in either schema
                if 'broadband' in freq and 'flatness_db' in freq['broadband']:
                    flatness = freq['broadband']['flatness_db']
                    if flatness > 15.0:
                        self.warning(f"Preset {preset_idx:02d}: very high flatness {flatness:.1f}dB")
                elif 'overall' in freq and 'flatness_std_db' in freq['overall']:
                    flatness = freq['overall']['flatness_std_db']
                    if flatness > 15.0:
                        self.warning(f"Preset {preset_idx:02d}: very high flatness {flatness:.1f}dB")

            except json.JSONDecodeError as e:
                invalid_freq.append((preset_idx, f"JSON parse error: {e}"))
            except Exception as e:
                invalid_freq.append((preset_idx, f"error: {e}"))

        if invalid_freq:
            for preset_idx, reason in invalid_freq:
                self.error(f"Preset {preset_idx:02d} frequency metrics invalid: {reason}")
        else:
            self.info(f"✓ All frequency metrics valid")

    def check_audio_files(self):
        """Validate audio file integrity (existence, non-zero size)."""
        empty_audio = []

        for preset_idx in range(self.EXPECTED_PRESETS):
            preset_dir = self.baseline_dir / f"preset_{preset_idx:02d}"

            for audio_file in ['wet.wav', 'dry.wav']:
                audio_path = preset_dir / audio_file

                if not audio_path.exists():
                    continue

                file_size = audio_path.stat().st_size

                if file_size == 0:
                    empty_audio.append((preset_idx, audio_file))
                elif file_size < 1024:  # Suspiciously small (< 1KB)
                    self.warning(f"Preset {preset_idx:02d}: {audio_file} very small ({file_size} bytes)")

        if empty_audio:
            for preset_idx, filename in empty_audio:
                self.error(f"Preset {preset_idx:02d}: {filename} is empty (0 bytes)")
        else:
            self.info(f"✓ All audio files have non-zero size")

    def check_data_consistency(self):
        """Check consistency between metadata and metrics."""
        inconsistencies = []

        for preset_idx in range(self.EXPECTED_PRESETS):
            preset_dir = self.baseline_dir / f"preset_{preset_idx:02d}"
            metadata_path = preset_dir / "metadata.json"
            rt60_path = preset_dir / "rt60_metrics.json"

            if not (metadata_path.exists() and rt60_path.exists()):
                continue

            try:
                with open(metadata_path, 'r') as f:
                    metadata = json.load(f)
                with open(rt60_path, 'r') as f:
                    rt60 = json.load(f)

                # Check sample rate consistency
                if 'sample_rate' in metadata and 'sample_rate' in rt60:
                    if metadata['sample_rate'] != rt60['sample_rate']:
                        inconsistencies.append(
                            (preset_idx,
                             f"sample_rate mismatch: metadata={metadata['sample_rate']}, rt60={rt60['sample_rate']}")
                        )

            except Exception as e:
                # Errors already reported in previous checks
                pass

        if inconsistencies:
            for preset_idx, reason in inconsistencies:
                self.error(f"Preset {preset_idx:02d}: {reason}")
        else:
            self.info(f"✓ Data consistency checks passed")

    def error(self, message: str):
        """Log an error."""
        self.errors.append(message)

    def warning(self, message: str):
        """Log a warning."""
        self.warnings.append(message)

    def info(self, message: str):
        """Log an info message."""
        self.info_messages.append(message)

    def print_summary(self):
        """Print validation summary."""
        print(f"\n{'='*60}")
        print(f"{BLUE}Validation Summary{RESET}")
        print(f"{'='*60}\n")

        # Info messages
        if self.info_messages:
            for msg in self.info_messages:
                print(f"{GREEN}{msg}{RESET}")

        # Warnings
        if self.warnings:
            print(f"\n{YELLOW}⚠ Warnings ({len(self.warnings)}):{RESET}")
            for msg in self.warnings:
                print(f"  {YELLOW}• {msg}{RESET}")

        # Errors
        if self.errors:
            print(f"\n{RED}✗ Errors ({len(self.errors)}):{RESET}")
            for msg in self.errors:
                print(f"  {RED}• {msg}{RESET}")

        # Final status
        print(f"\n{'='*60}")
        if self.errors:
            print(f"{RED}❌ VALIDATION FAILED{RESET}")
            print(f"   {len(self.errors)} error(s), {len(self.warnings)} warning(s)")
        else:
            print(f"{GREEN}✅ VALIDATION PASSED{RESET}")
            if self.warnings:
                print(f"   {len(self.warnings)} warning(s)")
        print(f"{'='*60}\n")


def compute_directory_checksum(baseline_dir: Path) -> str:
    """Compute SHA256 checksum of all JSON files in baseline directory."""
    checksums = []

    for preset_idx in range(37):
        for filename in ['metadata.json', 'rt60_metrics.json', 'freq_metrics.json']:
            file_path = baseline_dir / f"preset_{preset_idx:02d}" / filename
            if file_path.exists():
                with open(file_path, 'rb') as f:
                    file_hash = hashlib.sha256(f.read()).hexdigest()
                    checksums.append(file_hash)

    # Combine all file hashes and hash again
    combined = ''.join(sorted(checksums))
    return hashlib.sha256(combined.encode()).hexdigest()


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <baseline_dir>", file=sys.stderr)
        print(f"Example: {sys.argv[0]} test-results/baseline-ci", file=sys.stderr)
        return 2

    baseline_dir = sys.argv[1]

    # Run validation
    validator = BaselineValidator(baseline_dir)
    passed = validator.validate()

    # Compute directory checksum if validation passed
    if passed:
        checksum = compute_directory_checksum(Path(baseline_dir))
        print(f"{BLUE}Baseline checksum:{RESET} {checksum[:16]}...{checksum[-16:]}\n")

    return 0 if passed else 1


if __name__ == '__main__':
    sys.exit(main())
