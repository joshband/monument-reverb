# Test Output JSON Schemas

**Purpose:** Formal specification of all JSON output formats produced by the testing infrastructure.

**Version:** 1.0.0
**Last Updated:** 2026-01-08

---

## Overview

All Monument Reverb testing tools output JSON for machine-readable analysis and regression detection. This document defines the schema, validation rules, and examples for each output type.

**📁 Formal Schema Files:** Extracted JSON Schema files are available in [schemas/](schemas/) directory for automated validation.

## Quick Links

- **[schemas/README.md](schemas/README.md)** - Schema validation guide and usage examples
- **[schemas/rt60_metrics.schema.json](schemas/rt60_metrics.schema.json)** - RT60 decay time schema
- **[schemas/frequency_response.schema.json](schemas/frequency_response.schema.json)** - Frequency analysis schema
- **[schemas/capture_metadata.schema.json](schemas/capture_metadata.schema.json)** - Capture parameters schema
- **[schemas/regression_report.schema.json](schemas/regression_report.schema.json)** - Regression report schema
- **[schemas/cpu_profile.schema.json](schemas/cpu_profile.schema.json)** - CPU profiling schema

## Schema Catalog

| Schema | Producer | Consumer | Purpose |
|--------|----------|----------|---------|
| [RT60 Metrics](#rt60-metrics-schema) | `rt60_analysis_robust.py` | `compare_baseline.py`, CI | RT60 decay time measurements |
| [Frequency Response Metrics](#frequency-response-metrics-schema) | `frequency_response.py` | `compare_baseline.py`, CI | Spectral analysis results |
| [Capture Metadata](#capture-metadata-schema) | `monument_plugin_analyzer` | Documentation, debugging | Audio capture parameters |
| [Regression Report](#regression-report-schema) | `compare_baseline.py` | CI, visualization | Baseline comparison results |
| [CPU Profile Summary](#cpu-profile-summary-schema) | `analyze_profile.py` | Performance analysis | CPU bottleneck analysis |

---

## RT60 Metrics Schema

### Producer
`tools/plugin-analyzer/python/rt60_analysis_robust.py`

### Purpose
Reverb decay time (RT60) measurements per octave band using Schroeder integration method with fallback strategies.

### Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "timestamp", "input_file", "sample_rate", "duration_seconds", "broadband", "octave_bands"],
  "properties": {
    "version": {
      "type": "string",
      "description": "Schema version (semver)",
      "example": "1.0.0"
    },
    "timestamp": {
      "type": "string",
      "format": "date-time",
      "description": "Analysis timestamp (ISO 8601)",
      "example": "2026-01-08T14:30:45Z"
    },
    "input_file": {
      "type": "string",
      "description": "Path to analyzed WAV file",
      "example": "./test-results/preset_07/wet.wav"
    },
    "sample_rate": {
      "type": "integer",
      "description": "Audio sample rate (Hz)",
      "minimum": 44100,
      "maximum": 192000,
      "example": 48000
    },
    "duration_seconds": {
      "type": "number",
      "description": "Audio file duration (seconds)",
      "minimum": 0,
      "example": 30.0
    },
    "broadband": {
      "type": "object",
      "required": ["rt60_seconds", "confidence", "method"],
      "properties": {
        "rt60_seconds": {
          "type": "number",
          "description": "Overall RT60 decay time (seconds)",
          "minimum": 0,
          "example": 8.45
        },
        "confidence": {
          "type": "string",
          "enum": ["high", "medium", "low"],
          "description": "Confidence level of measurement",
          "example": "high"
        },
        "method": {
          "type": "string",
          "enum": ["schroeder", "manual_slope", "envelope", "fallback"],
          "description": "Method used for RT60 calculation",
          "example": "schroeder"
        }
      }
    },
    "octave_bands": {
      "type": "object",
      "description": "RT60 per octave band (Hz: {rt60_seconds, confidence, method})",
      "patternProperties": {
        "^[0-9]+$": {
          "type": "object",
          "required": ["rt60_seconds", "confidence", "method"],
          "properties": {
            "rt60_seconds": {
              "type": ["number", "null"],
              "description": "RT60 for this band (null if unmeasurable)",
              "minimum": 0
            },
            "confidence": {
              "type": "string",
              "enum": ["high", "medium", "low", "n/a"]
            },
            "method": {
              "type": "string"
            }
          }
        }
      },
      "example": {
        "125": {"rt60_seconds": 7.2, "confidence": "high", "method": "schroeder"},
        "250": {"rt60_seconds": 8.1, "confidence": "high", "method": "schroeder"},
        "500": {"rt60_seconds": 8.5, "confidence": "high", "method": "schroeder"},
        "1000": {"rt60_seconds": 8.7, "confidence": "high", "method": "schroeder"},
        "2000": {"rt60_seconds": 8.3, "confidence": "medium", "method": "manual_slope"},
        "4000": {"rt60_seconds": 7.8, "confidence": "medium", "method": "manual_slope"}
      }
    },
    "analysis_notes": {
      "type": "string",
      "description": "Optional analysis warnings or notes",
      "example": "High-frequency bands used fallback method due to low SNR"
    }
  }
}
```

### Example Output

```json
{
  "version": "1.0.0",
  "timestamp": "2026-01-08T14:30:45Z",
  "input_file": "./test-results/preset_07/wet.wav",
  "sample_rate": 48000,
  "duration_seconds": 30.0,
  "broadband": {
    "rt60_seconds": 8.45,
    "confidence": "high",
    "method": "schroeder"
  },
  "octave_bands": {
    "125": {"rt60_seconds": 7.2, "confidence": "high", "method": "schroeder"},
    "250": {"rt60_seconds": 8.1, "confidence": "high", "method": "schroeder"},
    "500": {"rt60_seconds": 8.5, "confidence": "high", "method": "schroeder"},
    "1000": {"rt60_seconds": 8.7, "confidence": "high", "method": "schroeder"},
    "2000": {"rt60_seconds": 8.3, "confidence": "medium", "method": "manual_slope"},
    "4000": {"rt60_seconds": 7.8, "confidence": "medium", "method": "manual_slope"}
  },
  "analysis_notes": ""
}
```

### Validation Rules

1. **RT60 range:** 0.1s ≤ rt60_seconds ≤ 60s (typical reverbs)
2. **Confidence levels:**
   - `high`: SNR >40dB, clean decay curve
   - `medium`: SNR 20-40dB, acceptable decay
   - `low`: SNR <20dB, noisy measurement
3. **Octave bands:** Standard bands (125, 250, 500, 1k, 2k, 4k, 8k Hz)
4. **Null values:** Acceptable for bands where RT60 unmeasurable (e.g., very short decay, high noise)

---

## Frequency Response Metrics Schema

### Producer
`tools/plugin-analyzer/python/frequency_response.py`

### Purpose
Spectral analysis via FFT, measuring magnitude response, flatness, and quality rating.

### Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "timestamp", "input_file", "sample_rate", "broadband", "octave_bands", "quality_rating"],
  "properties": {
    "version": {
      "type": "string",
      "description": "Schema version",
      "example": "1.0.0"
    },
    "timestamp": {
      "type": "string",
      "format": "date-time"
    },
    "input_file": {
      "type": "string"
    },
    "sample_rate": {
      "type": "integer",
      "minimum": 44100
    },
    "broadband": {
      "type": "object",
      "required": ["flatness_db", "mean_gain_db"],
      "properties": {
        "flatness_db": {
          "type": "number",
          "description": "Overall frequency flatness (±dB std dev)",
          "example": 3.2
        },
        "mean_gain_db": {
          "type": "number",
          "description": "Average gain across all frequencies",
          "example": -0.5
        },
        "peak_frequency_hz": {
          "type": ["number", "null"],
          "description": "Frequency of maximum gain",
          "example": 2450
        },
        "notch_frequency_hz": {
          "type": ["number", "null"],
          "description": "Frequency of maximum attenuation",
          "example": 8120
        }
      }
    },
    "octave_bands": {
      "type": "object",
      "description": "Gain per octave band (Hz: {gain_db, flatness_db})",
      "patternProperties": {
        "^[0-9]+$": {
          "type": "object",
          "required": ["gain_db", "flatness_db"],
          "properties": {
            "gain_db": {
              "type": "number",
              "description": "Average gain in this band (dB)"
            },
            "flatness_db": {
              "type": "number",
              "description": "Standard deviation within band (dB)"
            }
          }
        }
      },
      "example": {
        "125": {"gain_db": -1.2, "flatness_db": 0.8},
        "250": {"gain_db": -0.5, "flatness_db": 1.1},
        "500": {"gain_db": 0.1, "flatness_db": 0.9}
      }
    },
    "quality_rating": {
      "type": "string",
      "enum": ["Excellent", "Good", "Fair", "Colored"],
      "description": "Overall frequency response quality",
      "example": "Excellent"
    }
  }
}
```

### Example Output

```json
{
  "version": "1.0.0",
  "timestamp": "2026-01-08T14:31:20Z",
  "input_file": "./test-results/preset_07/wet.wav",
  "sample_rate": 48000,
  "broadband": {
    "flatness_db": 3.2,
    "mean_gain_db": -0.5,
    "peak_frequency_hz": 2450,
    "notch_frequency_hz": null
  },
  "octave_bands": {
    "125": {"gain_db": -1.2, "flatness_db": 0.8},
    "250": {"gain_db": -0.5, "flatness_db": 1.1},
    "500": {"gain_db": 0.1, "flatness_db": 0.9},
    "1000": {"gain_db": 0.3, "flatness_db": 1.0},
    "2000": {"gain_db": 0.8, "flatness_db": 1.5},
    "4000": {"gain_db": -0.2, "flatness_db": 1.2},
    "8000": {"gain_db": -1.0, "flatness_db": 1.8},
    "12800": {"gain_db": -2.5, "flatness_db": 2.3}
  },
  "quality_rating": "Excellent"
}
```

### Quality Rating Criteria

| Rating | Flatness (±dB) | Description |
|--------|---------------|-------------|
| **Excellent** | ≤±3dB | Very flat, natural response |
| **Good** | ±3-6dB | Slight coloration, acceptable |
| **Fair** | ±6-10dB | Noticeable coloration |
| **Colored** | >±10dB | Heavy coloration, intentional or problematic |

---

## Capture Metadata Schema

### Producer
`tools/plugin-analyzer/src/AudioCapture.cpp`

### Purpose
Document audio capture parameters for reproducibility and debugging.

### Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "timestamp", "plugin_path", "test_type", "duration_seconds", "sample_rate", "num_channels", "block_size"],
  "properties": {
    "version": {"type": "string"},
    "timestamp": {"type": "string", "format": "date-time"},
    "plugin_path": {
      "type": "string",
      "description": "Path to VST3/AU plugin",
      "example": "/Users/user/Library/Audio/Plug-Ins/VST3/Monument.vst3"
    },
    "preset_index": {
      "type": ["integer", "null"],
      "description": "Preset index (0-based), null if default",
      "minimum": 0,
      "example": 7
    },
    "preset_name": {
      "type": ["string", "null"],
      "description": "Preset name if available",
      "example": "Cathedral Hall"
    },
    "test_type": {
      "type": "string",
      "enum": ["impulse", "sweep", "noise", "pink"],
      "description": "Test signal type",
      "example": "impulse"
    },
    "duration_seconds": {
      "type": "number",
      "minimum": 0,
      "example": 30.0
    },
    "sample_rate": {
      "type": "integer",
      "enum": [44100, 48000, 88200, 96000, 176400, 192000],
      "example": 48000
    },
    "num_channels": {
      "type": "integer",
      "enum": [1, 2],
      "example": 2
    },
    "block_size": {
      "type": "integer",
      "description": "Audio processing block size (samples)",
      "enum": [64, 128, 256, 512, 1024, 2048],
      "example": 512
    },
    "output_files": {
      "type": "object",
      "properties": {
        "dry": {"type": "string", "example": "dry.wav"},
        "wet": {"type": "string", "example": "wet.wav"}
      }
    }
  }
}
```

### Example Output

```json
{
  "version": "1.0.0",
  "timestamp": "2026-01-08T14:25:00Z",
  "plugin_path": "/Users/user/Library/Audio/Plug-Ins/VST3/Monument.vst3",
  "preset_index": 7,
  "preset_name": "Cathedral Hall",
  "test_type": "impulse",
  "duration_seconds": 30.0,
  "sample_rate": 48000,
  "num_channels": 2,
  "block_size": 512,
  "output_files": {
    "dry": "dry.wav",
    "wet": "wet.wav"
  }
}
```

---

## Regression Report Schema

### Producer
`tools/compare_baseline.py`

### Purpose
Detect audio regressions by comparing current captures against baseline data.

### Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "timestamp", "baseline_dir", "current_dir", "threshold", "overall_pass", "preset_results"],
  "properties": {
    "version": {"type": "string"},
    "timestamp": {"type": "string", "format": "date-time"},
    "baseline_dir": {
      "type": "string",
      "description": "Path to baseline data directory"
    },
    "current_dir": {
      "type": "string",
      "description": "Path to current test results"
    },
    "threshold": {
      "type": "number",
      "description": "Regression threshold (0.01 = 1%)",
      "minimum": 0,
      "maximum": 1,
      "example": 0.01
    },
    "overall_pass": {
      "type": "boolean",
      "description": "True if all presets passed"
    },
    "preset_results": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["preset_index", "preset_name", "pass", "rt60_deviation_percent", "freq_flatness_delta_db", "waveform_correlation"],
        "properties": {
          "preset_index": {"type": "integer", "minimum": 0},
          "preset_name": {"type": "string"},
          "pass": {"type": "boolean"},
          "rt60_deviation_percent": {
            "type": "number",
            "description": "RT60 % change vs. baseline"
          },
          "freq_flatness_delta_db": {
            "type": "number",
            "description": "Flatness change (dB)"
          },
          "waveform_correlation": {
            "type": "number",
            "minimum": -1,
            "maximum": 1,
            "description": "Waveform correlation coefficient (>0.95 expected)"
          },
          "rms_difference_db": {
            "type": "number",
            "description": "RMS waveform difference (dB)"
          },
          "spectral_difference_db": {
            "type": "number",
            "description": "PSD difference (dB)"
          },
          "failures": {
            "type": "array",
            "items": {"type": "string"},
            "description": "List of failed checks",
            "example": ["RT60 deviation 5.2% exceeds threshold 1.0%"]
          }
        }
      }
    },
    "summary": {
      "type": "object",
      "properties": {
        "total_presets": {"type": "integer"},
        "passed": {"type": "integer"},
        "failed": {"type": "integer"},
        "worst_rt60_deviation_percent": {"type": "number"},
        "worst_correlation": {"type": "number"}
      }
    }
  }
}
```

### Example Output

```json
{
  "version": "1.0.0",
  "timestamp": "2026-01-08T14:35:00Z",
  "baseline_dir": "./test-results/baseline-v1.0.0",
  "current_dir": "./test-results/preset-baseline",
  "threshold": 0.01,
  "overall_pass": true,
  "preset_results": [
    {
      "preset_index": 0,
      "preset_name": "Small Room",
      "pass": true,
      "rt60_deviation_percent": 0.5,
      "freq_flatness_delta_db": 0.2,
      "waveform_correlation": 0.998,
      "rms_difference_db": -42.5,
      "spectral_difference_db": -38.2,
      "failures": []
    },
    {
      "preset_index": 7,
      "preset_name": "Cathedral Hall",
      "pass": false,
      "rt60_deviation_percent": 5.2,
      "freq_flatness_delta_db": 1.8,
      "waveform_correlation": 0.932,
      "rms_difference_db": -28.3,
      "spectral_difference_db": -22.1,
      "failures": [
        "RT60 deviation 5.2% exceeds threshold 1.0%",
        "Waveform correlation 0.932 below threshold 0.95"
      ]
    }
  ],
  "summary": {
    "total_presets": 37,
    "passed": 36,
    "failed": 1,
    "worst_rt60_deviation_percent": 5.2,
    "worst_correlation": 0.932
  }
}
```

### Pass Criteria

**Preset passes if ALL conditions met:**
- RT60 deviation ≤ threshold (default: 1%)
- Waveform correlation ≥ 0.95
- RMS difference <-30dB (low energy difference)
- Spectral difference <-20dB (similar frequency content)

---

## CPU Profile Summary Schema

### Producer
`scripts/analyze_profile.py`

### Purpose
Identify CPU bottlenecks from Xcode Instruments trace data.

### Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "type": "object",
  "required": ["version", "timestamp", "profile_duration_seconds", "top_functions", "module_breakdown", "estimated_cpu_load_percent"],
  "properties": {
    "version": {"type": "string"},
    "timestamp": {"type": "string", "format": "date-time"},
    "profile_duration_seconds": {
      "type": "number",
      "description": "Total profiling duration",
      "example": 30.0
    },
    "top_functions": {
      "type": "array",
      "description": "Top CPU-consuming functions",
      "items": {
        "type": "object",
        "required": ["function_name", "source_file", "time_ms", "percent_of_total"],
        "properties": {
          "function_name": {"type": "string"},
          "source_file": {"type": "string"},
          "time_ms": {"type": "number"},
          "percent_of_total": {"type": "number"}
        }
      },
      "maxItems": 30
    },
    "module_breakdown": {
      "type": "object",
      "description": "CPU time per DSP module",
      "patternProperties": {
        "^[A-Za-z_]+$": {
          "type": "object",
          "properties": {
            "time_ms": {"type": "number"},
            "percent_of_total": {"type": "number"}
          }
        }
      },
      "example": {
        "TubeRayTracer": {"time_ms": 1250.3, "percent_of_total": 35.2},
        "Chambers": {"time_ms": 890.5, "percent_of_total": 25.1}
      }
    },
    "estimated_cpu_load_percent": {
      "type": "number",
      "description": "Estimated % CPU at 512 samples/block, 48kHz",
      "minimum": 0,
      "maximum": 100,
      "example": 8.5
    },
    "optimization_recommendations": {
      "type": "array",
      "items": {"type": "string"},
      "example": [
        "TubeRayTracer: Consider SIMD optimization (>1% CPU)",
        "Chambers: High CPU usage, review allpass chain length"
      ]
    }
  }
}
```

### Example Output

```json
{
  "version": "1.0.0",
  "timestamp": "2026-01-08T15:00:00Z",
  "profile_duration_seconds": 30.0,
  "top_functions": [
    {
      "function_name": "processBlock",
      "source_file": "TubeRayTracer.cpp",
      "time_ms": 1250.3,
      "percent_of_total": 35.2
    },
    {
      "function_name": "processAllpass",
      "source_file": "Chambers.cpp",
      "time_ms": 890.5,
      "percent_of_total": 25.1
    }
  ],
  "module_breakdown": {
    "TubeRayTracer": {"time_ms": 1250.3, "percent_of_total": 35.2},
    "Chambers": {"time_ms": 890.5, "percent_of_total": 25.1},
    "MemoryEchoes": {"time_ms": 450.2, "percent_of_total": 12.7},
    "ModulationMatrix": {"time_ms": 320.1, "percent_of_total": 9.0}
  },
  "estimated_cpu_load_percent": 8.5,
  "optimization_recommendations": [
    "TubeRayTracer: Consider SIMD optimization (>1% CPU)",
    "Chambers: High CPU usage, review allpass chain length"
  ]
}
```

---

## Validation Tools

### Python JSON Schema Validation

```bash
# Install jsonschema
pip3 install jsonschema

# Validate RT60 output
python3 -c "
import json, jsonschema
schema = json.load(open('docs/schemas/rt60_metrics.schema.json'))
data = json.load(open('test-results/preset_07/rt60_metrics.json'))
jsonschema.validate(data, schema)
print('✓ Valid')
"
```

### Shell jq Validation

```bash
# Check required fields exist
jq -e '.version, .broadband.rt60_seconds, .octave_bands' \
  test-results/preset_07/rt60_metrics.json

# Validate RT60 range (0.1-60s)
jq '.broadband.rt60_seconds | if . < 0.1 or . > 60 then error("RT60 out of range") else . end' \
  test-results/preset_07/rt60_metrics.json
```

---

## Schema Versioning

### Version Policy

- **Major** (X.0.0): Breaking changes (field removal, type changes)
- **Minor** (1.X.0): Additive changes (new optional fields)
- **Patch** (1.0.X): Clarifications, documentation

### Compatibility

- Consumers MUST ignore unknown fields (forward compatibility)
- Producers SHOULD include `version` field in all outputs
- Consumers SHOULD validate against min supported version

### Migration Path

When updating schemas:
1. Document breaking changes in [CHANGELOG.md](../CHANGELOG.md)
2. Provide migration script if needed
3. Update baseline data with new schema
4. Increment version appropriately

---

## See Also

- [docs/testing_audit.md](testing_audit.md) - Testing infrastructure audit
- [docs/TESTING_GUIDE.md](TESTING_GUIDE.md) - Comprehensive testing guide
- [tools/compare_baseline.py](../tools/compare_baseline.py) - Regression detection tool
- [JSON Schema Specification](https://json-schema.org/) - JSON Schema documentation

---

**Schema Version:** 1.0.0
**Last Updated:** 2026-01-08
