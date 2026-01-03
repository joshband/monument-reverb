# Testing and Validation

## pluginval

Use pluginval to run automated validation on the built plugin.

```sh
PLUGINVAL=/path/to/pluginval \
BUILD_DIR=build CONFIG=Release \
./scripts/run_pluginval.sh
```

- Override `PLUGIN_PATH` if you want to validate AU instead of VST3.
- Reports are written to `build/pluginval-report` by default.
- Check the report for NaN/denormal flags, parameter automation issues, and stress test failures.

## Memory Echoes harness (isolated)

Build the standalone harness to iterate on Memory Echoes outside the plugin:

```sh
cmake -S . -B build -G Ninja -DMONUMENT_ENABLE_TESTS=ON
cmake --build build --target monument_memory_echoes_harness
```

Run the harness to render recall-only audio:

```sh
build/monument_memory_echoes_harness_artefacts/Release/monument_memory_echoes_harness \
  --seconds 40 --memory 0.7 --depth 0.6 --decay 0.5 --drift 0.3 --signal piano
```

Optional:
- `--input path.wav` to feed a file instead of the generated test bursts.
- `--mix-input` to include the original input in the output render.
- `--signal pad|pluck|piano` to switch the built-in generator.

## Performance benchmarking (REAPER)

- Open REAPER, load a single Monument instance, and open Performance Monitor.
- Duplicate the track to reach 50-100 instances.
- Compare CPU usage against other reference reverbs at the same buffer size.
- While running audio, toggle Freeze and switch presets to confirm no clicks or spikes.

## Debug/Profiling hooks

Set `MONUMENT_TESTING` to log peak sample values and per-block CPU time:

```sh
cmake -S . -B build -G Xcode -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_CXX_FLAGS="-DMONUMENT_TESTING=1"
```

Example log line:

```
Monument MONUMENT_TESTING peak=0.892314 blockMs=0.312
```

## Leak and sanitizer checks

- macOS: Instruments (Leaks, Time Profiler) on the AU/VST3 host process.
- Clang AddressSanitizer: enable `-fsanitize=address` in your build (Debug recommended).
- Linux: Valgrind on pluginval or a standalone test host.

## Manual stress checklist

- Preset switching during playback (including while Freeze is active).
- Extreme automation passes for Time, Mass, Density, Warp, Drift, and Bloom.
- Pillars mode/shape sweeps while monitoring output peaks.
