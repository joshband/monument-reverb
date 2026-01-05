#!/usr/bin/env python3
"""
Analyze Monument CPU profiling data from xctrace export
"""

import xml.etree.ElementTree as ET
import sys
from collections import defaultdict
import re

def parse_weight(weight_str):
    """Parse weight string like '1.00 ms' to microseconds."""
    if not weight_str:
        return 0
    # Extract numeric value and unit
    match = re.search(r'([\d.]+)\s*(\w+)', weight_str)
    if not match:
        return 0

    value = float(match.group(1))
    unit = match.group(2).lower()

    # Convert to microseconds
    if unit in ['ms', 'milliseconds']:
        return value * 1000  # ms to μs
    elif unit in ['s', 'seconds']:
        return value * 1000000  # s to μs
    elif unit in ['us', 'μs', 'microseconds']:
        return value
    elif unit in ['ns', 'nanoseconds']:
        return value / 1000  # ns to μs

    return 0

def parse_profile_xml(xml_path):
    """Parse the xctrace XML export and extract CPU usage by function."""

    print(f"Parsing profile data from: {xml_path}")
    tree = ET.parse(xml_path)
    root = tree.getroot()

    # Function times: function_name -> total_time_us
    function_times = defaultdict(float)

    # Process monument binary frames only
    total_samples = 0
    monument_samples = 0

    # Build a map of binary IDs to their names for reference resolution
    binary_names = {}
    for binary in root.findall(".//binary[@name]"):
        binary_id = binary.get('id')
        binary_name = binary.get('name')
        if binary_id and binary_name:
            binary_names[binary_id] = binary_name

    print(f"Loaded {len(binary_names)} binaries")
    monument_ids = [k for k, v in binary_names.items() if v == "Monument"]
    print(f"Monument binary IDs: {monument_ids}")

    for row in root.findall(".//row"):
        total_samples += 1

        # Get weight (time) for this sample
        weight_elem = row.find(".//weight")
        if weight_elem is None:
            continue

        weight_str = weight_elem.get('fmt', '')
        weight_us = parse_weight(weight_str)

        if weight_us == 0:
            continue

        # Get backtrace
        backtrace = row.find(".//backtrace")
        if backtrace is None:
            continue

        # Look at each frame in the backtrace
        monument_frame_found = False
        for frame in backtrace.findall(".//frame"):
            # Get function name
            func_name = frame.get('name', '')

            # Get binary info (handle both inline and ref cases)
            binary = frame.find(".//binary")
            binary_name = None
            if binary is not None:
                # Check if it's a reference or inline definition
                binary_ref = binary.get('ref')
                if binary_ref and binary_ref in binary_names:
                    binary_name = binary_names[binary_ref]
                else:
                    binary_name = binary.get('name', '')

                # Only track Monument binary functions
                if binary_name == "Monument":
                    monument_frame_found = True

                    # Get source file for better context
                    source = frame.find(".//source/path")
                    if source is not None:
                        source_path = source.text or ''
                        # Extract just filename from path
                        if '/' in source_path:
                            source_file = source_path.split('/')[-1]
                        else:
                            source_file = source_path

                        # Create a more readable function name
                        if source_file and func_name:
                            func_display = f"{source_file}: {func_name}"
                        elif func_name:
                            func_display = func_name
                        else:
                            func_display = f"{source_file}: <unknown>"
                    else:
                        # No source info - use address if available
                        addr = frame.get('addr', '')
                        if func_name and not func_name.startswith('0x'):
                            func_display = func_name
                        elif addr:
                            func_display = f"Monument+{addr}"
                        else:
                            func_display = "Monument: <unknown>"

                    # Accumulate time for this function
                    function_times[func_display] += weight_us

        if monument_frame_found:
            monument_samples += 1

    print(f"Total samples: {total_samples}")
    print(f"Monument samples: {monument_samples}")
    print(f"Monument functions found: {len(function_times)}")

    return function_times, total_samples, monument_samples

def analyze_monument_functions(function_times, total_samples, monument_samples):
    """Analyze Monument-specific functions and DSP modules."""

    if not function_times:
        print("\nNo Monument functions found in profile data")
        print("This might happen if:")
        print("- Audio wasn't actively processing during the profile")
        print("- The app was idle during profiling")
        return

    # Calculate total time (in seconds)
    total_time_us = sum(function_times.values())
    total_time_ms = total_time_us / 1000
    total_time_s = total_time_ms / 1000

    # Estimate total profiling duration (30 seconds)
    profiling_duration_s = 30.0

    print("\n" + "="*80)
    print("Monument CPU Profile Analysis")
    print("="*80)
    print(f"\nTotal profiling duration: ~{profiling_duration_s:.1f} seconds")
    print(f"Total Monument CPU time: {total_time_ms:.2f} ms")
    print(f"Average CPU load: {(total_time_s / profiling_duration_s * 100):.2f}%")
    print()

    # Sort by time (actual CPU usage)
    sorted_functions = sorted(function_times.items(),
                             key=lambda x: x[1],
                             reverse=True)

    print("Top 30 Monument Functions by CPU Time:")
    print("-" * 80)
    print(f"{'Function':<60} {'Time (ms)':<12} {'% of Total':<12}")
    print("-" * 80)

    # Monument DSP modules to track
    dsp_modules = {
        'TubeRayTracer': 0.0,
        'Chambers': 0.0,
        'ElasticHallway': 0.0,
        'AlienAmplification': 0.0,
        'MemoryEchoes': 0.0,
        'ModulationMatrix': 0.0,
        'MacroMapper': 0.0,
        'ExpressiveMacroMapper': 0.0,
        'DspRoutingGraph': 0.0,
        'processBlock': 0.0,
        'AllpassDiffuser': 0.0,
        'ParameterSmoother': 0.0,
    }

    for i, (func, time_us) in enumerate(sorted_functions[:30]):
        time_ms = time_us / 1000
        percentage = (time_us / total_time_us * 100) if total_time_us > 0 else 0

        # Truncate long function names
        display_name = func[:57] + "..." if len(func) > 60 else func

        print(f"{display_name:<60} {time_ms:>8.2f}    {percentage:>8.2f}%")

        # Track DSP module times
        for module in dsp_modules.keys():
            if module in func:
                dsp_modules[module] += time_us

        if i == 29:
            print("-" * 80)

    # Show DSP module breakdown
    print("\nMonument DSP Module Breakdown:")
    print("-" * 80)
    print(f"{'Module':<30} {'Time (ms)':<12} {'% of Total':<12} {'Est. CPU %'}")
    print("-" * 80)

    sorted_modules = sorted(dsp_modules.items(), key=lambda x: x[1], reverse=True)
    for module, time_us in sorted_modules:
        if time_us > 0:
            time_ms = time_us / 1000
            percentage = (time_us / total_time_us * 100) if total_time_us > 0 else 0
            est_cpu = (time_us / 1000000 / profiling_duration_s * 100)  # Convert to % of total time
            print(f"{module:<30} {time_ms:>8.2f}    {percentage:>8.2f}%     {est_cpu:>8.2f}%")

    print("-" * 80)
    print()

    # Provide analysis
    print("Analysis & Recommendations:")
    print("-" * 80)

    tube_time_us = dsp_modules.get('TubeRayTracer', 0)
    if tube_time_us > 0:
        tube_ms = tube_time_us / 1000
        tube_cpu_pct = (tube_time_us / 1000000 / profiling_duration_s * 100)
        print(f"✓ TubeRayTracer found: {tube_ms:.2f} ms total (~{tube_cpu_pct:.2f}% CPU)")

        if tube_cpu_pct > 1.0:
            print(f"  → SIMD optimization recommended (current: {tube_cpu_pct:.2f}%, target: 0.8%)")
            print(f"  → Estimated savings: {tube_cpu_pct * 0.47:.2f}% CPU (47% reduction)")
        else:
            print(f"  → TubeRayTracer is already efficient (<1% CPU)")
            print(f"  → SIMD optimization would only save ~{tube_cpu_pct * 0.47:.3f}% CPU")
    else:
        print("✗ TubeRayTracer not found in profile")
        print("  → Audio may not have been actively processing")
        print("  → Try profiling again with audio input and knob adjustments")

    # Check other modules
    print()
    other_modules = [(k, v) for k, v in sorted_modules if k != 'TubeRayTracer' and v > tube_time_us]
    if other_modules:
        print("Higher CPU modules to consider optimizing:")
        for module, time_us in other_modules[:3]:
            time_ms = time_us / 1000
            cpu_pct = (time_us / 1000000 / profiling_duration_s * 100)
            print(f"  • {module}: {time_ms:.2f} ms (~{cpu_pct:.2f}% CPU)")

    print()

def main():
    if len(sys.argv) < 2:
        print("Usage: analyze_profile.py <profile_export.xml>")
        sys.exit(1)

    xml_path = sys.argv[1]

    try:
        function_times, total_samples, monument_samples = parse_profile_xml(xml_path)

        if function_times:
            analyze_monument_functions(function_times, total_samples, monument_samples)
        else:
            print("\nWarning: No Monument functions found in profile")
            print("This might happen if:")
            print("- Audio wasn't actively processing during the profile")
            print("- The app was idle during profiling")
            print("- Try running the profile again with active audio processing")
    except Exception as e:
        print(f"\nError parsing profile: {e}")
        import traceback
        traceback.print_exc()

if __name__ == "__main__":
    main()
