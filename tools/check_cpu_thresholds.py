#!/usr/bin/env python3
"""
CPU Performance Threshold Checker
Enforces maximum CPU usage limits per DSP module and overall plugin.

Exit Codes:
  0 - All thresholds passed
  1 - One or more thresholds exceeded
  2 - Invalid input or error

Usage:
  python3 tools/check_cpu_thresholds.py <cpu_profile.json>
  python3 tools/check_cpu_thresholds.py test-results/cpu_profile.json
"""

import json
import sys
from pathlib import Path
from typing import Dict, List, Tuple
from datetime import datetime

# ANSI color codes for output formatting
class Colors:
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BLUE = '\033[94m'
    BOLD = '\033[1m'
    RESET = '\033[0m'

# CPU usage thresholds (% of total CPU time)
CPU_THRESHOLDS = {
    "TubeRayTracer": 40.0,  # Max 40% CPU
    "Chambers": 30.0,       # Max 30% CPU
    "MemoryEchoes": 15.0,   # Max 15% CPU
}

# Overall plugin threshold @ 512 samples/block, 48kHz
OVERALL_THRESHOLD_PERCENT = 10.0  # Max 10% CPU

def load_cpu_profile(profile_path: Path) -> Dict:
    """Load and validate CPU profile JSON."""
    if not profile_path.exists():
        print(f"{Colors.RED}✗ Error: Profile file not found: {profile_path}{Colors.RESET}")
        sys.exit(2)

    try:
        with open(profile_path, 'r') as f:
            data = json.load(f)

        # Validate required fields
        required_fields = ["version", "timestamp", "module_breakdown", "estimated_cpu_load_percent"]
        for field in required_fields:
            if field not in data:
                print(f"{Colors.RED}✗ Error: Missing required field '{field}' in profile{Colors.RESET}")
                sys.exit(2)

        return data

    except json.JSONDecodeError as e:
        print(f"{Colors.RED}✗ Error: Invalid JSON in profile file: {e}{Colors.RESET}")
        sys.exit(2)
    except Exception as e:
        print(f"{Colors.RED}✗ Error reading profile: {e}{Colors.RESET}")
        sys.exit(2)

def check_module_thresholds(module_breakdown: Dict[str, Dict]) -> Tuple[bool, List[str]]:
    """
    Check CPU usage for each DSP module against thresholds.

    Returns:
        (all_passed, violations) tuple
    """
    violations = []
    all_passed = True

    print(f"\n{Colors.BOLD}Module CPU Usage:{Colors.RESET}")
    print(f"{'Module':<20} {'CPU %':<10} {'Threshold':<12} {'Status':<10}")
    print("-" * 60)

    # Check each module that has a threshold
    for module_name, threshold in CPU_THRESHOLDS.items():
        if module_name in module_breakdown:
            module_data = module_breakdown[module_name]
            cpu_percent = module_data.get("percent_of_total", 0.0)

            if cpu_percent > threshold:
                status = f"{Colors.RED}✗ FAIL{Colors.RESET}"
                violation_msg = f"{module_name}: {cpu_percent:.1f}% exceeds threshold {threshold:.1f}%"
                violations.append(violation_msg)
                all_passed = False
            else:
                status = f"{Colors.GREEN}✓ PASS{Colors.RESET}"

            print(f"{module_name:<20} {cpu_percent:<10.1f} {f'<= {threshold:.1f}%':<12} {status}")
        else:
            # Module not present in profile (0% CPU)
            status = f"{Colors.BLUE}○ N/A{Colors.RESET}"
            print(f"{module_name:<20} {0.0:<10.1f} {f'<= {threshold:.1f}%':<12} {status}")

    # Report other modules without thresholds (informational)
    other_modules = set(module_breakdown.keys()) - set(CPU_THRESHOLDS.keys())
    if other_modules:
        print(f"\n{Colors.BOLD}Other Modules (informational):{Colors.RESET}")
        for module_name in sorted(other_modules):
            cpu_percent = module_breakdown[module_name].get("percent_of_total", 0.0)
            print(f"{module_name:<20} {cpu_percent:<10.1f}")

    return all_passed, violations

def check_overall_threshold(estimated_cpu_percent: float) -> Tuple[bool, List[str]]:
    """
    Check overall plugin CPU usage against threshold.

    Returns:
        (passed, violations) tuple
    """
    violations = []

    print(f"\n{Colors.BOLD}Overall Plugin CPU Usage:{Colors.RESET}")
    print(f"{'Metric':<30} {'Value':<15} {'Threshold':<15} {'Status':<10}")
    print("-" * 80)

    if estimated_cpu_percent > OVERALL_THRESHOLD_PERCENT:
        status = f"{Colors.RED}✗ FAIL{Colors.RESET}"
        violation_msg = f"Overall CPU: {estimated_cpu_percent:.2f}% exceeds threshold {OVERALL_THRESHOLD_PERCENT:.1f}%"
        violations.append(violation_msg)
        passed = False
    else:
        status = f"{Colors.GREEN}✓ PASS{Colors.RESET}"
        passed = True

    print(f"{'Estimated CPU @ 512/48kHz':<30} {f'{estimated_cpu_percent:.2f}%':<15} "
          f"{f'<= {OVERALL_THRESHOLD_PERCENT:.1f}%':<15} {status}")

    return passed, violations

def print_summary(all_passed: bool, all_violations: List[str], profile_path: Path, profile_data: Dict):
    """Print summary of threshold checks."""
    print(f"\n{Colors.BOLD}{'=' * 80}{Colors.RESET}")
    print(f"{Colors.BOLD}CPU THRESHOLD CHECK SUMMARY{Colors.RESET}")
    print(f"{Colors.BOLD}{'=' * 80}{Colors.RESET}")

    print(f"\n{Colors.BOLD}Profile Information:{Colors.RESET}")
    print(f"  File: {profile_path}")
    print(f"  Version: {profile_data.get('version', 'unknown')}")
    print(f"  Timestamp: {profile_data.get('timestamp', 'unknown')}")
    print(f"  Duration: {profile_data.get('profile_duration_seconds', 0):.1f}s")

    if all_passed:
        print(f"\n{Colors.GREEN}{Colors.BOLD}✓ ALL THRESHOLDS PASSED{Colors.RESET}")
        print(f"{Colors.GREEN}No CPU performance violations detected.{Colors.RESET}")
    else:
        print(f"\n{Colors.RED}{Colors.BOLD}✗ THRESHOLD VIOLATIONS DETECTED{Colors.RESET}")
        print(f"\n{Colors.RED}Violations ({len(all_violations)}):{Colors.RESET}")
        for i, violation in enumerate(all_violations, 1):
            print(f"  {i}. {violation}")

        print(f"\n{Colors.YELLOW}Recommended Actions:{Colors.RESET}")
        print(f"  1. Profile the specific modules that exceeded thresholds")
        print(f"  2. Look for optimization opportunities (SIMD, caching, etc.)")
        print(f"  3. Consider reducing algorithmic complexity")
        print(f"  4. Review parameter ranges that trigger high CPU usage")

        # Provide module-specific recommendations
        for violation in all_violations:
            if "TubeRayTracer" in violation:
                print(f"\n{Colors.YELLOW}TubeRayTracer optimization ideas:{Colors.RESET}")
                print(f"  - Vectorize ray intersection tests with SIMD")
                print(f"  - Reduce ray count or bounces per sample")
                print(f"  - Pre-compute geometry acceleration structures")
            elif "Chambers" in violation:
                print(f"\n{Colors.YELLOW}Chambers optimization ideas:{Colors.RESET}")
                print(f"  - Vectorize 8×8 matrix multiplication")
                print(f"  - Reduce allpass filter chain length")
                print(f"  - Cache frequently-used filter coefficients")
            elif "MemoryEchoes" in violation:
                print(f"\n{Colors.YELLOW}MemoryEchoes optimization ideas:{Colors.RESET}")
                print(f"  - Optimize delay line access patterns")
                print(f"  - Use SIMD for multi-tap processing")
                print(f"  - Reduce interpolation complexity")

    print(f"\n{Colors.BOLD}{'=' * 80}{Colors.RESET}")

def main():
    """Main entry point for CPU threshold checker."""
    if len(sys.argv) != 2:
        print(f"{Colors.RED}Usage: python3 {sys.argv[0]} <cpu_profile.json>{Colors.RESET}")
        print(f"\nExample:")
        print(f"  python3 {sys.argv[0]} test-results/cpu_profile.json")
        sys.exit(2)

    profile_path = Path(sys.argv[1])

    print(f"{Colors.BOLD}Monument Reverb - CPU Threshold Checker{Colors.RESET}")
    print(f"Profile: {profile_path}")
    print(f"Time: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")

    # Load profile data
    profile_data = load_cpu_profile(profile_path)

    # Check module thresholds
    module_passed, module_violations = check_module_thresholds(
        profile_data["module_breakdown"]
    )

    # Check overall threshold
    overall_passed, overall_violations = check_overall_threshold(
        profile_data["estimated_cpu_load_percent"]
    )

    # Combine results
    all_passed = module_passed and overall_passed
    all_violations = module_violations + overall_violations

    # Print summary
    print_summary(all_passed, all_violations, profile_path, profile_data)

    # Exit with appropriate code
    sys.exit(0 if all_passed else 1)

if __name__ == "__main__":
    main()
