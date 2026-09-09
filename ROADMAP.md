# Monument Reverb - Project Roadmap

**Purpose:** Long-term vision, future enhancements, and module ideation

---

## Current Phase (Stable Foundation)

**Status:** DSP architecture documentation is substantially complete — see
[docs/architecture/dsp/00-index.md](docs/architecture/dsp/00-index.md) for the
authoritative per-module list and completion state; don't infer a module
count from this file. See [TESTING.md](TESTING.md) for current test status —
don't infer it from this file either, which tracks vision/ideation, not test
counts.

**Focus:** Documentation completion, performance optimization

### Near-Term Milestones (0-2 Months)

**Critical RT-Safety Fixes** (from 2026-01-07/08 reviews):

1. ✅ **Playhead null check** - `getPlayHead()` in `plugin/PluginProcessor.cpp`'s
   `processBlock()` is guarded before use (guards against a null playhead by
   falling back to an empty `PositionInfo`) — file line numbers drift as the
   file grows, so no line reference is pinned here; verify against source.
2. ✅ **Timeline-preset / TubeRayTracer allocation on the audio thread** -
   Fixed; proven allocation-free by
   `monument_realtime_allocation_characterization_test` (see `TESTING.md`).
3. ✅ **ModulationMatrix lock-free** - Fixed with an announce-then-verify
   reader handshake over rotating snapshot slots (no `SpinLock`); proven
   race-free under ThreadSanitizer by
   `tests/ModulationMatrixConcurrencyStressTest.cpp` (see `ARCHITECTURE.md`).

**Documentation Completion:**

- Both DSP module docs that were previously outstanding now exist and are
  substantial: Chambers
  ([docs/architecture/dsp/core-modules/03-chambers.md](docs/architecture/dsp/core-modules/03-chambers.md))
  and Ancient Monuments
  ([docs/architecture/dsp/control-systems/00-ancient-monuments.md](docs/architecture/dsp/control-systems/00-ancient-monuments.md) —
  note it lives under `control-systems/` as `00-`, not the `07-` core-module
  numbering once assumed for it).
- Remaining documentation gaps (if any) are tracked in
  [docs/architecture/dsp/00-index.md](docs/architecture/dsp/00-index.md)
  itself, not here — that index carries a per-module status column and is
  the source of truth, since a count hardcoded into this vision doc will
  only go stale again.

**Build & Test Hygiene:**

- ✅ CTest config-aware paths
- ✅ Dead code removal: deleted the unused `ExperimentalModulation` module
  (never wired into the shipped plugin) and `DspRoutingGraph`'s generic
  series/parallel/feedback/crossfeed graph executor (the host-visible
  `routingPreset` parameter only ever drove its bypass mask, never that
  executor) — see `ARCHITECTURE.md`.
- ⏳ Particle system RTTI removal (if playground becomes production UI) - no
  particle system currently exists in this repository; this stays
  conditional on that playground work happening at all.
- ⏳ Fast particle removal (swap-pop instead of O(n) removal) - same caveat
  as above.
- **Target:** all registered CTest targets green — see [TESTING.md](TESTING.md) for the current inventory and status, not a fixed count here

**Asset Pipeline Decision (unmade — not planned work):**

- No decision has been made yet. This is an open question sitting in front
  of the team, not a scheduled task with an owner or timeline.
- Current: Split between binary-embedded (knobs) and file-based (presets)
- Options:
  - Binary-embedded: Larger plugin, no file dependencies
  - File-based: Smaller plugin, requires resource installation
  - Hybrid: Core assets embedded, expansions file-based
- **Action Required:** Make the decision, then document and implement the
  chosen strategy. Until that happens, treat the current binary/file split
  as unintentional, not as an endorsed hybrid design.

---

## Phase Ideation (Not Yet Planned)

### Phase 5: Performance & Optimization (Future)

**Goal:** Achieve production-grade CPU efficiency and real-time safety

**Enhancements:**

1. **SIMD Vectorization** - 15-25% CPU reduction
   - AVX-optimized matrix multiplication (8×8 FDN)
   - Vectorized fractional delay interpolation
   - Batch spatial distance calculations
   
2. **Further Lock-Free Hardening** - Extend lock-free patterns beyond what
   already shipped (ModulationMatrix's own announce-then-verify rotating
   snapshot handshake is done — see Near-Term Milestones above — so this is
   about what's left, not a restatement of it)
   - Pre-allocated routing configurations with atomic swap (for routing
     preset switching)
   - Lock-free parameter updates

3. **Memory Optimization**
   - Relaxed memory ordering for independent parameters
   - Bitmask-based smoother tracking
   - Cache-aligned buffer allocation

**Target:** <6% CPU @ 64 samples, zero audio dropouts

---

### Phase 6: UI Enhancement (Ideation)

**Goal:** Production-ready plugin UI with visualizations

**Options Under Consideration:**

1. **Option A: Macro-Only Interface** (from 2026-01-07 implementation plan)
   - 3 macro knobs (Scale, Character, Breath)
   - Preset browser with visual profiles
   - Minimal CPU overhead
   - Fast development timeline

2. **Option B: Full Parameter Interface**
   - All 25+ parameters exposed
   - Advanced users can dive deep
   - Complexity management challenge

3. **Option C: Hybrid Interface** (Recommended)
   - Default: Macro-only view
   - Advanced: Expandable parameter sections
   - Best of both worlds

**UI Component System (from implementation plan):**

Components:
- `LayeredKnob` - Variants: geode/metal/industrial; States: default/hover/dragging
- `ParticleField` - States: off/idle/reactive; Variants: embers/smoke/sparks
- `StatusBar` - Shows audio state and metrics
- `MacroCluster` - 10 macros, macro-only layout with labels
- `MacroVisualOverlay` - OpenGL rings + glyph hints from JSON profiles

Interaction Model:
- **States:** idle, editing (dragging), reactive (audio peaks), debug (visualization overlays)
- **Transitions:**
  - Hover → highlight ring
  - Drag → knob rotates with velocity smoothing
  - Audio peak → particle burst + glow intensification
  - Cursor move → emitter follows with soft easing

**Design Tokens (W3C-compliant):**

```json
{
  "color": {
    "surface": { "value": "#0B0D10" },
    "surfaceAlt": { "value": "#14181F" },
    "textPrimary": { "value": "#F2F2F2" },
    "textSecondary": { "value": "#B7B9C2" },
    "accentWarm": { "value": "#E07A3F" },
    "accentCool": { "value": "#57B7C7" }
  },
  "space": {
    "xs": { "value": "4px" },
    "sm": { "value": "8px" },
    "md": { "value": "16px" },
    "lg": { "value": "24px" }
  },
  "radius": {
    "sm": { "value": "6px" },
    "md": { "value": "10px" },
    "lg": { "value": "16px" }
  },
  "motion": {
    "fast": { "value": "120ms" },
    "base": { "value": "180ms" },
    "slow": { "value": "300ms" }
  }
}
```

**Accessibility:**
- Keyboard focus traversal for knobs
- Visible focus ring
- Screen reader support (JUCE accessibility API)
- Status text for mode changes

**Performance Targets:**
- 60fps UI refresh rate
- Particles toggleable for low-power systems
- Audio-reactive bursts correlated with RMS/peak

**Failure Modes:**
- Missing asset packs → fallback test pattern, disable pack switching
- High CPU load → particle throttling and glow clamp
- UI/Audio mismatch → status banner with update indicators

---

### Phase 7: Advanced DSP Modules (Ideation)

**New Module Ideas:**

1. **Spectral Freeze** - Frequency-domain delay network
   - FFT-based reverb tail manipulation
   - Harmonic freezing and morphing
   - Integration with MemoryEchoes

2. **Convolution Layer** - Hybrid algorithmic + IR reverb
   - Short IRs for early reflections
   - Algorithmic for infinite tail
   - IR morphing capabilities

3. **Granular Reverb** - Micro-sample manipulation
   - Grain clouds from reverb tail
   - Pitch/time stretching
   - Experimental textures

4. **Neural Reverb** - ML-based room modeling
   - Learned room responses
   - Real-time inference (<5% CPU)
   - Custom room training

5. **Multiband Processing** - Frequency-split reverb
   - Independent decay per band
   - Crossover control
   - Tonal shaping

**Integration Considerations:**
- Modular routing (insert into existing signal flow)
- Preset compatibility (backward compatible)
- CPU budget management

---

### Phase 8: Preset Expansion (Ideation)

**Current:** 37 factory presets (`plugin/PresetManager.h`'s
`kNumFactoryPresets`: 18 original + 5 "Living" (Phase 3) + 5 Physical
Modeling (Phase 5) + 9 "Living" (Phase 6))

**Target:** 50+ factory presets across categories

**Preset Categories:**

1. **Spaces** (10-15 presets)
   - Cathedral, Hall, Chamber, Room, Plate
   - Realistic architectural reverbs

2. **Creative** (10-15 presets)
   - Shimmer, Reverse, Infinite, Freeze
   - Experimental/musical effects

3. **Physical** (8-10 presets)
   - Spring, Tube, Elastic, Alien
   - Physical modeling showcases

4. **Memory** (5-8 presets)
   - Echoes, Ghosts, Layers, Abyss
   - MemoryEchoes system focus

5. **Modulation** (8-10 presets)
   - Chorus, Flanger, Vibrato, Detune
   - Modulation-heavy textures

6. **Utility** (5-8 presets)
   - Subtle Enhancement, Ambience, Tight Room
   - Mix-ready presets

**Preset Features:**
- Visual profiles (particle system colors/behaviors)
- Macro mappings (unique per preset)
- Automation lanes (timeline sequences)
- Artist presets (guest sound designers)

---

### Phase 9: Distribution & Ecosystem (Future)

**Goal:** Release-ready packaging and ecosystem integration

**Milestones:**

1. **Code Signing & Notarization**
   - Apple Developer ID
   - Windows Authenticode
   - AAX signing (Avid account)

2. **Plugin Formats**
   - VST3 ✅ (working)
   - AU ✅ (working)
   - AAX (Avid Developer Program)
   - CLAP (future consideration)

3. **Host Compatibility**
   - Ableton Live, Logic Pro, Pro Tools
   - FL Studio, Cubase, Reaper, Studio One
   - Compatibility testing suite

4. **Documentation & Marketing**
   - User manual (PDF + web)
   - Video tutorials
   - Sound demos and audio examples
   - Website with interactive demos

5. **Distribution Channels**
   - Direct sales (website)
   - Plugin marketplaces (Plugin Boutique, etc.)
   - Subscription services (Splice, Output Hub)

6. **Licensing & Protection**
   - License key system
   - Hardware dongle (optional)
   - Trial/demo version

---

## Future Module Enhancements

### MemoryEchoes Expansion

**Current:** Dual buffer (24s + 180s), probabilistic recall. Now
permanently wired into the live signal path (reached unconditionally,
around Chambers) rather than prepared-but-unreached — see `ARCHITECTURE.md`'s
"Memory Echoes is reached, wired around Chambers" section. The enhancements
below build on that live module, not a hypothetical future connection.

**Enhancements:**

1. **Energy-Age Tracking** - Remember which memories are "loudest"
2. **Multi-Resolution Buffers** - 4+ time scales (short/medium/long/infinite)
3. **Spectral Freezing** - Freeze specific frequency bands
4. **SIMD Optimization** - Reduce CPU overhead
5. **User-Configurable Durations** - Parameters for buffer lengths

### Chambers Enhancements

**Current:** 8×8 FDN with Hadamard/Householder matrix

**Enhancements:**

1. **Adaptive Matrix** - Change topology based on input
2. **Nonlinear Feedback** - Soft clipping, saturation per line
3. **Time-Variant Delays** - Modulated delay line lengths
4. **Allpass Chains** - Additional diffusion in feedback
5. **SIMD Matrix Ops** - 4x speedup with AVX

### SpatialProcessor Enhancements

**Current:** 3D positioning with Doppler shift

**Enhancements:**

1. **HRTF Processing** - Binaural spatial audio
2. **Room Acoustics** - Wall reflections, absorption
3. **Motion Paths** - Automated spatial movement
4. **Multi-Source** - Multiple sound sources in space
5. **Ambisonics Output** - Higher-order spatial audio

### ModulationMatrix Enhancements

**Current:** 4 sources × 27 destinations

**Enhancements:**

1. **User LFOs** - 8+ user-configurable modulation sources
2. **Envelope Followers** - Multiple audio-rate followers
3. **MIDI Modulation** - MIDI CC, velocity, aftertouch
4. **Modulation Curves** - Non-linear mapping curves
5. **Modulation Visualization** - Real-time modulation display

---

## Research & Experimentation

### Areas of Interest

1. **Machine Learning Integration**
   - Room IR prediction from parameters
   - Preset recommendation system
   - Adaptive processing based on input

2. **Physical Modeling**
   - String/membrane resonators
   - Waveguide networks
   - Finite element room modeling

3. **Psychoacoustic Enhancement**
   - Perceptual loudness matching
   - Stereo image optimization
   - Clarity enhancement algorithms

4. **Real-Time Analysis**
   - Automatic gain staging
   - Mix context awareness
   - Adaptive parameter suggestions

5. **Cloud Integration**
   - Preset cloud sync
   - Collaborative sound design
   - Community preset sharing

---

## Long-Term Vision (2-5 Years)

**Monument Reverb Ecosystem:**

1. **Monument Suite** - Family of spatial plugins
   - Monument Reverb (flagship)
   - Monument Delay (advanced delay network)
   - Monument Modulation (creative effects)
   - Monument Spatial (3D audio processor)

2. **Monument Live** - Performance-optimized version
   - Reduced CPU overhead
   - Live-friendly presets
   - MIDI control surface support

3. **Monument Creative** - Experimental version
   - All experimental modules enabled
   - No stability guarantees
   - Playground for sound designers

4. **Monument SDK** - Developer toolkit
   - Custom module development
   - Preset scripting language
   - Visual profile editor

---

## Community & Open Source (Consideration)

**Potential Open Source Components:**

- DSP module implementations (educational)
- Preset format specification
- Visual profile JSON schema
- Test suite and benchmarking tools

**Community Features:**

- User preset submission system
- Sound design contests
- Educational content creation
- Forum and Discord community

---

**For current session tracking, see [NEXT_SESSION_HANDOFF.md](docs/archive/NEXT_SESSION_HANDOFF.md)**

**For implementation status, see [docs/STATUS.md](docs/archive/sessions/STATUS.md)**

**For detailed session history, see [CHANGELOG.md](CHANGELOG.md)**
