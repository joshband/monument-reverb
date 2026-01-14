# Monument Reverb JSON Schemas

**Version:** 1.0.0
**Last Updated:** 2026-01-08

## Overview

This directory contains formal JSON Schema specifications for all test output formats used in the Monument Reverb testing infrastructure. These schemas enable automated validation of test outputs to ensure data integrity.

## Schema Files

| Schema File | Description | Producer | Consumer |
|------------|-------------|----------|----------|
| [rt60_metrics.schema.json](rt60_metrics.schema.json) | RT60 decay time measurements | `rt60_analysis_robust.py` | `compare_baseline.py`, CI |
| [frequency_response.schema.json](frequency_response.schema.json) | Spectral analysis results | `frequency_response.py` | `compare_baseline.py`, CI |
| [capture_metadata.schema.json](capture_metadata.schema.json) | Audio capture parameters | `monument_plugin_analyzer` | Documentation, debugging |
| [regression_report.schema.json](regression_report.schema.json) | Baseline comparison results | `compare_baseline.py` | CI, visualization |
| [cpu_profile.schema.json](cpu_profile.schema.json) | CPU bottleneck analysis | `analyze_profile.py` | Performance analysis |
| [spatial_metrics.schema.json](spatial_metrics.schema.json) | Spatial cue metrics (ITD/ILD/IACC) | `spatial_metrics.py` | Analysis, CI |

## Usage

### Python Validation (jsonschema)

```bash
# Install jsonschema package
pip3 install jsonschema

# Validate a test output file
python3 -c "
import json
from jsonschema import validate

schema = json.load(open('docs/schemas/rt60_metrics.schema.json'))
data = json.load(open('test-results/preset_07/rt60_metrics.json'))
validate(instance=data, schema=schema)
print('✓ Valid')
"
```

### Command-Line Validation (jq)

```bash
# Check if file is valid JSON
jq empty test-results/preset_07/rt60_metrics.json

# Check required fields exist
jq -e '.version, .broadband.rt60_seconds, .octave_bands' \
  test-results/preset_07/rt60_metrics.json
```

### Automated Validation Tool

Use the schema validation tool (Phase 3 deliverable):

```bash
# Validate all test outputs against schemas
python3 tools/validate_schemas.py test-results/preset-baseline/

# Validate specific preset
python3 tools/validate_schemas.py test-results/preset-baseline/preset_07/
```

## Schema Versioning

All schemas follow **Semantic Versioning** (semver):

- **Major (X.0.0):** Breaking changes (field removal, type changes)
- **Minor (1.X.0):** Additive changes (new optional fields)
- **Patch (1.0.X):** Clarifications, documentation

### Current Version: 1.0.0

All schemas are at version 1.0.0 as of 2026-01-08.

## Compatibility Rules

**For Producers (tools that generate JSON):**
- MUST include `version` field in all outputs
- MUST comply with all `required` fields
- SHOULD include optional fields when data is available
- MAY add custom fields (prefixed with `x_` to avoid conflicts)

**For Consumers (tools that read JSON):**
- MUST ignore unknown fields (forward compatibility)
- SHOULD validate against schema before processing
- SHOULD handle missing optional fields gracefully
- MUST check version compatibility

## Schema Details

### RT60 Metrics
- **Required fields:** version, timestamp, input_file, sample_rate, duration_seconds, broadband, octave_bands
- **Validation rules:**
  - RT60 range: 0.1s ≤ rt60_seconds ≤ 60s
  - Confidence levels: high, medium, low, n/a
  - Standard octave bands: 125, 250, 500, 1k, 2k, 4k, 8k Hz

### Frequency Response
- **Required fields:** version, timestamp, input_file, sample_rate, broadband, octave_bands, quality_rating
- **Quality ratings:**
  - **Excellent:** ≤±3dB (very flat, natural response)
  - **Good:** ±3-6dB (slight coloration, acceptable)
  - **Fair:** ±6-10dB (noticeable coloration)
  - **Colored:** >±10dB (heavy coloration)

### Capture Metadata
- **Required fields:** version, timestamp, plugin_path, test_type, duration_seconds, sample_rate, num_channels, block_size
- **Test types:** impulse, sweep, noise, pink
- **Sample rates:** 44.1k, 48k, 88.2k, 96k, 176.4k, 192k

### Regression Report
- **Required fields:** version, timestamp, baseline_dir, current_dir, threshold, overall_pass, preset_results
- **Pass criteria:**
  - RT60 deviation ≤ threshold (default: 1%)
  - Waveform correlation ≥ 0.95
  - RMS difference <-30dB
  - Spectral difference <-20dB

### CPU Profile
- **Required fields:** version, timestamp, profile_duration_seconds, top_functions, module_breakdown, estimated_cpu_load_percent
- **Top functions:** Limited to 30 entries
- **CPU load:** Estimated % at 512 samples/block, 48kHz

## See Also

- [../test_output_schemas.md](../test_output_schemas.md) - Detailed schema documentation with examples
- [../../tools/validate_baseline.py](../../tools/validate_baseline.py) - Baseline data validator (Phase 2)
- [../../tools/validate_schemas.py](../../tools/validate_schemas.py) - Schema validator (Phase 3, TODO)
- [JSON Schema Specification](https://json-schema.org/draft-07/schema) - JSON Schema Draft 07 docs

---

**Status:** ✅ Phase 3 - Step 1 Complete (JSON Schema Extraction)
**Next Step:** Implement `tools/validate_schemas.py` for automated validation
