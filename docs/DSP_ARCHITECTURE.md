# Monument DSP Architecture

Monument is built as a modular reverb system designed for extreme scale and abstract, impossible spaces. Each module is real-time safe: no dynamic memory allocation, no locks, and no logging inside `process()`.

## Signal Flow

1. **Foundation (input conditioning)**
   Normalizes input, removes DC, manages headroom, and prepares the signal for diffusion. Optional drive or tilt EQ lives here.

2. **Pillars (early reflection clusters)**
   Creates dense early reflections using clustered multi-taps and micro-delays. Focuses on shaping perceived space size and initial depth.

3. **Chambers (FDN reverb core)**
   The main feedback delay network: high-order, matrix-mixed delay lines with absorption and decay control for large-scale, dense spaces.

4. **Weathering (modulation and drift)**
   Modulates delay lengths, diffusion, and filters with slow LFOs and random drift to avoid metallic artifacts and to keep large spaces alive.

5. **Buttress (safety and feedback control)**
   Protects stability: feedback limiting, saturation, and energy control to prevent runaway feedback while preserving tail density.

6. **Facade (output imaging)**
   Stereo imaging, mid/side control, final EQ, and wet/dry output shaping. Handles the final presentation of the space.

## ASCII Audio Path Diagram

```
Input
  |
  v
[Foundation] -> [Pillars] -> [Chambers] -> [Weathering] -> [Buttress] -> [Facade]
  |
  +----------------------------- Dry Tap -------------------------------> Mix -> Output
```

## Module Responsibilities

- **Foundation**: input gain staging, DC removal, pre-emphasis, mono/stereo conditioning.
- **Pillars**: early reflection density, diffusion clusters, stereo scatter/offsets.
- **Chambers**: primary reverb energy and size, high-order FDN core, absorption filters.
- **Weathering**: modulation, drift, and slow movement to prevent static tails.
- **Buttress**: safety limiter for feedback loops, energy balance, optional freeze gating.
- **Facade**: output imaging, tone shaping, wet/dry mix, final trim.

## Real-time Safety Rules

- No dynamic allocation in `process()`.
- No locks, logging, or file IO on the audio thread.
- All large buffers and delay lines are allocated in `prepare()`.
