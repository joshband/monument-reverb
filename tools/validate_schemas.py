#!/usr/bin/env python3
"""
JSON Schema Validator for Monument Reverb Test Outputs

Validates test output JSON files against formal JSON Schema specifications.

Usage:
    python3 tools/validate_schemas.py <directory>
    python3 tools/validate_schemas.py --verbose <directory>
    python3 tools/validate_schemas.py --schema rt60_metrics <file.json>

Exit codes:
    0 - All validations passed
    1 - One or more validation failures
    2 - Error (missing schemas, invalid JSON, etc.)

Examples:
    # Validate all baseline data
    python3 tools/validate_schemas.py test-results/baseline-ci/

    # Validate specific preset with verbose output
    python3 tools/validate_schemas.py --verbose test-results/preset-baseline/preset_07/

    # Validate single file against specific schema
    python3 tools/validate_schemas.py --schema rt60_metrics test.json
"""

import json
import sys
import os
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import argparse

try:
    import jsonschema
    from jsonschema import validate, ValidationError, Draft7Validator
except ImportError:
    print("ERROR: jsonschema package not installed", file=sys.stderr)
    print("Install with: pip3 install jsonschema", file=sys.stderr)
    sys.exit(2)


# ANSI color codes
class Colors:
    GREEN = '\033[92m'
    RED = '\033[91m'
    YELLOW = '\033[93m'
    BLUE = '\033[94m'
    RESET = '\033[0m'
    BOLD = '\033[1m'


def load_schemas(schemas_dir: Path) -> Dict[str, dict]:
    """Load all JSON schemas from the schemas directory."""
    schemas = {}

    if not schemas_dir.exists():
        print(f"{Colors.RED}ERROR: Schemas directory not found: {schemas_dir}{Colors.RESET}", file=sys.stderr)
        sys.exit(2)

    schema_files = list(schemas_dir.glob("*.schema.json"))
    if not schema_files:
        print(f"{Colors.RED}ERROR: No schema files found in {schemas_dir}{Colors.RESET}", file=sys.stderr)
        sys.exit(2)

    for schema_file in schema_files:
        schema_name = schema_file.stem.replace('.schema', '')
        try:
            with open(schema_file, 'r') as f:
                schema = json.load(f)
                # Validate the schema itself
                Draft7Validator.check_schema(schema)
                schemas[schema_name] = schema
        except json.JSONDecodeError as e:
            print(f"{Colors.RED}ERROR: Invalid JSON in schema {schema_file}: {e}{Colors.RESET}", file=sys.stderr)
            sys.exit(2)
        except Exception as e:
            print(f"{Colors.RED}ERROR: Failed to load schema {schema_file}: {e}{Colors.RESET}", file=sys.stderr)
            sys.exit(2)

    return schemas


def detect_schema_type(data: dict, file_path: Path) -> Optional[str]:
    """
    Auto-detect which schema to use based on file structure and naming.

    Detection strategy:
    1. Check filename patterns (rt60_metrics.json, frequency_response.json, etc.)
    2. Check required fields unique to each schema
    3. Check file location patterns
    """
    filename = file_path.name.lower()

    # Strategy 1: Filename patterns
    if 'rt60' in filename or filename == 'rt60_metrics.json':
        return 'rt60_metrics'
    elif 'freq' in filename or 'frequency' in filename or filename == 'frequency_response.json':
        return 'frequency_response'
    elif 'capture' in filename or filename == 'capture_metadata.json' or filename == 'metadata.json':
        return 'capture_metadata'
    elif 'regression' in filename or filename == 'regression_report.json':
        return 'regression_report'
    elif 'cpu' in filename or 'profile' in filename:
        return 'cpu_profile'

    # Strategy 2: Required fields (more robust)
    if 'octave_bands' in data and 'broadband' in data:
        if 'rt60_seconds' in data.get('broadband', {}):
            return 'rt60_metrics'
        elif 'flatness_db' in data.get('broadband', {}):
            return 'frequency_response'

    if 'plugin_path' in data and 'test_type' in data and 'block_size' in data:
        return 'capture_metadata'

    if 'baseline_dir' in data and 'preset_results' in data and 'overall_pass' in data:
        return 'regression_report'

    if 'top_functions' in data and 'module_breakdown' in data and 'estimated_cpu_load_percent' in data:
        return 'cpu_profile'

    return None


def validate_file(file_path: Path, schemas: Dict[str, dict], schema_type: Optional[str] = None, verbose: bool = False) -> Tuple[bool, str]:
    """
    Validate a single JSON file against the appropriate schema.

    Returns:
        (success: bool, message: str)
    """
    try:
        with open(file_path, 'r') as f:
            data = json.load(f)
    except json.JSONDecodeError as e:
        return False, f"Invalid JSON: {e}"
    except Exception as e:
        return False, f"Failed to read file: {e}"

    # Detect schema type if not specified
    if schema_type is None:
        schema_type = detect_schema_type(data, file_path)
        if schema_type is None:
            return False, "Could not auto-detect schema type"

    # Check if schema exists
    if schema_type not in schemas:
        available = ', '.join(schemas.keys())
        return False, f"Schema '{schema_type}' not found. Available: {available}"

    schema = schemas[schema_type]

    # Validate against schema
    try:
        validate(instance=data, schema=schema)
        return True, f"Valid {schema_type}"
    except ValidationError as e:
        error_path = '.'.join(str(p) for p in e.path) if e.path else 'root'
        if verbose:
            return False, f"Schema validation failed at '{error_path}': {e.message}"
        else:
            return False, f"Schema validation failed: {e.message[:100]}"


def validate_directory(directory: Path, schemas: Dict[str, dict], verbose: bool = False) -> Tuple[int, int, List[str]]:
    """
    Recursively validate all JSON files in directory.

    Returns:
        (passed_count, failed_count, error_messages)
    """
    passed = 0
    failed = 0
    errors = []

    # Find all JSON files (excluding schema files)
    json_files = []
    for json_file in directory.rglob("*.json"):
        # Skip schema files
        if 'schemas' in json_file.parts:
            continue
        # Skip manifest files and other config
        if json_file.name in ['manifest.json', 'package.json']:
            continue
        json_files.append(json_file)

    if not json_files:
        print(f"{Colors.YELLOW}WARNING: No JSON files found in {directory}{Colors.RESET}")
        return 0, 0, []

    for json_file in sorted(json_files):
        rel_path = json_file.relative_to(directory)
        success, message = validate_file(json_file, schemas, verbose=verbose)

        if success:
            passed += 1
            if verbose:
                print(f"{Colors.GREEN}✓{Colors.RESET} {rel_path}: {message}")
        else:
            failed += 1
            error_msg = f"{rel_path}: {message}"
            errors.append(error_msg)
            print(f"{Colors.RED}✗{Colors.RESET} {error_msg}")

    return passed, failed, errors


def main():
    parser = argparse.ArgumentParser(
        description="Validate test output JSON files against schemas",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Validate all files in baseline directory
  %(prog)s test-results/baseline-ci/

  # Validate with verbose output
  %(prog)s --verbose test-results/preset-baseline/

  # Validate specific file with explicit schema
  %(prog)s --schema rt60_metrics test-results/preset_07/rt60_metrics.json
        """
    )
    parser.add_argument('path', type=str, help='Directory or file to validate')
    parser.add_argument('--schema', type=str, help='Schema type to use (auto-detect if not specified)')
    parser.add_argument('--verbose', '-v', action='store_true', help='Verbose output')
    parser.add_argument('--schemas-dir', type=str, default='docs/schemas',
                        help='Directory containing schema files (default: docs/schemas)')

    args = parser.parse_args()

    # Determine project root (assume script is in tools/)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    schemas_dir = project_root / args.schemas_dir

    # Load schemas
    if args.verbose:
        print(f"{Colors.BLUE}Loading schemas from {schemas_dir}...{Colors.RESET}")

    schemas = load_schemas(schemas_dir)

    if args.verbose:
        print(f"{Colors.BLUE}Loaded {len(schemas)} schemas: {', '.join(schemas.keys())}{Colors.RESET}\n")

    # Validate path
    path = Path(args.path)
    if not path.exists():
        print(f"{Colors.RED}ERROR: Path not found: {path}{Colors.RESET}", file=sys.stderr)
        sys.exit(2)

    # Validate file or directory
    if path.is_file():
        # Single file validation
        success, message = validate_file(path, schemas, schema_type=args.schema, verbose=args.verbose)
        if success:
            print(f"{Colors.GREEN}✓{Colors.RESET} {path.name}: {message}")
            sys.exit(0)
        else:
            print(f"{Colors.RED}✗{Colors.RESET} {path.name}: {message}")
            sys.exit(1)

    elif path.is_dir():
        # Directory validation
        print(f"{Colors.BOLD}Validating JSON files in: {path}{Colors.RESET}\n")

        passed, failed, errors = validate_directory(path, schemas, verbose=args.verbose)

        # Summary
        print(f"\n{Colors.BOLD}Summary:{Colors.RESET}")
        print(f"{Colors.GREEN}✓ Passed:{Colors.RESET} {passed}")
        print(f"{Colors.RED}✗ Failed:{Colors.RESET} {failed}")
        print(f"Total: {passed + failed}")

        if failed > 0:
            print(f"\n{Colors.RED}Validation failed for {failed} file(s){Colors.RESET}")
            sys.exit(1)
        else:
            print(f"\n{Colors.GREEN}All validations passed!{Colors.RESET}")
            sys.exit(0)

    else:
        print(f"{Colors.RED}ERROR: Path is not a file or directory: {path}{Colors.RESET}", file=sys.stderr)
        sys.exit(2)


if __name__ == '__main__':
    main()
