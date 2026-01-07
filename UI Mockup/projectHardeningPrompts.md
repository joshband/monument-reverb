Below is a **Claude Code–ready, implementation-plan–driven prompt set**, **prioritized in the exact order I recommend you run them**. These are designed to be pasted directly into Claude Code as **separate tasks** (or a single phased plan if you prefer), and they assume the ZIP project you just downloaded is the baseline.

I’m being opinionated here to minimize thrash and rework.

---

## 🥇 PRIORITY 1 — Project Hardening & BinaryData Wiring (Foundational)

**Goal:** Make the project compile cleanly, load assets deterministically, and establish non-negotiable conventions.

You are a JUCE C++ implementation agent.

TASK:
Audit and harden this JUCE demo project.

OBJECTIVES:

1. Verify BinaryData naming matches on-disk Assets folder paths (no flattening).
2. Implement a robust AssetManager that:

   * Loads all panel, header, and knob assets explicitly
   * Uses semantic keys (e.g. "panel.macro.bg", "knob.stone.heavy")
   * Fails loudly if an asset key is missing
3. Ensure all Components pull images ONLY via AssetManager.
4. Add compile-time comments documenting asset conventions and naming rules.

CONSTRAINTS:

* Do not change folder structure.
* Do not introduce randomness.
* Code must compile in JUCE 7+.
* Favor clarity over cleverness.

OUTPUT:

* Updated AssetManager.h/.cpp
* Any required fixes in Components that reference BinaryData
* Brief inline comments explaining decisions

---

## 🥈 PRIORITY 2 — Deterministic Knob Taxonomy (Visual Identity Lock)

**Goal:** Lock parameter → material identity so knobs never drift stylistically.

You are a JUCE UI systems agent.

TASK:
Implement a deterministic knob taxonomy system.

OBJECTIVES:

1. Create a KnobStyle struct that selects:

   * Stone variant
   * Crystal variant
   * Core variant
2. Implement a single authoritative function:
   KnobStyle styleForParameter(const String& paramID);
3. Update StoneKnob so:

   * It NEVER hashes randomly
   * All visual variation comes ONLY from styleForParameter
4. Encode the following intent:

   * Macro / Chaos parameters → heavier, fractured stone
   * Temporal parameters → obsidian / polished
   * Utility parameters → subdued, low-glow

CONSTRAINTS:

* No randomness
* Same parameter name must always map to same visual
* Easy to extend later with themes

OUTPUT:

* styleForParameter implementation
* Updated StoneKnob constructor
* Clear comments explaining taxonomy philosophy

---

## 🥉 PRIORITY 3 — Advanced Knob Animation Engine (Feel & Physics)

**Goal:** Replace naïve rotation with physical, gesture-aware motion.

You are a JUCE interaction & animation agent.

TASK:
Implement advanced knob animation for StoneKnob.

OBJECTIVES:

1. Add a visual-only physics layer:

   * visualValue
   * velocity
   * stiffness / damping
2. Implement gesture-dependent behavior:

   * Mouse: precise, stiff
   * Touch: fluid, forgiving
   * MIDI: snappy, authoritative
3. Replace linear value→angle mapping with a non-linear perceptual curve.
4. Ensure animation runs at a bounded rate (≤ 60Hz, prefer 30Hz).
5. Knob must remain automation-safe (audio value unchanged).

CONSTRAINTS:

* No audio-thread work
* No repaint storms
* Code must be readable and debuggable

OUTPUT:

* Updated StoneKnob.h/.cpp
* Any helper math functions
* Inline explanation of chosen constants

---

## 🟢 PRIORITY 4 — Preset Morphing Visual Layer (Continuity)

**Goal:** Preset changes feel continuous and intentional, not abrupt.

You are a JUCE state & UX agent.

TASK:
Implement visual-only preset morphing.

OBJECTIVES:

1. Create a VisualParam shadow state per parameter:

   * current (visual)
   * target (from APVTS)
2. On preset load:

   * Audio parameters update immediately
   * Visual targets update
   * Visual current interpolates smoothly
3. Drive StoneKnob visuals from visual state, not raw parameter value.
4. Add a subtle visual accent during morphing (e.g. slight glow increase).

CONSTRAINTS:

* Presets must remain 100% compatible
* No DSP changes
* Morphing must complete in ~300–500ms

OUTPUT:

* VisualParam system
* Editor timer loop for interpolation
* StoneKnob hookup changes

---

## 🔵 PRIORITY 5 — Audio-Reactive Micro-Modulation (Tasteful)

**Goal:** UI reacts to sound *without becoming a light show*.

You are a JUCE audio-visual integration agent.

TASK:
Add subtle audio-reactive visuals driven by reverb energy.

OBJECTIVES:

1. On DSP side:

   * Compute RMS or late-reverb energy
   * Expose via atomic<float>
2. On UI side:

   * Smooth energy at ~30Hz
   * Compress perceptually (sqrt / log)
3. Apply energy subtly to:

   * Knob crystal glow (weighted by parameter type)
   * Panel lighting (much weaker than knobs)
4. Ensure zero flicker and zero audio-thread painting.

CONSTRAINTS:

* Panels react slower than knobs
* No hue changes, only intensity
* CPU impact < 2%

OUTPUT:

* Processor energy tap
* UI smoothing logic
* Modified paint routines with comments

---

## 🟣 PRIORITY 6 — Theme System (Ancient / Void / Cathedral)

**Goal:** Material swaps, not layout forks.

You are a JUCE theming & architecture agent.

TASK:
Implement a theme system.

OBJECTIVES:

1. Define a Theme struct controlling:

   * Stone set
   * Crystal set
   * Glow intensity
   * Background color
2. Implement three built-in themes:

   * Ancient
   * Void
   * Cathedral
3. Ensure theme switching:

   * Does not affect parameters
   * Reuses existing assets
   * Triggers repaint only where needed

CONSTRAINTS:

* No duplication of Components
* No preset interaction
* Future-proof for AUv3

OUTPUT:

* Theme definitions
* AssetManager integration
* Editor-level theme switch hook

---

## 🧠 How to Use These Prompts

**Recommended workflow:**

* Run them **one at a time**, in order
* Commit after each priority
* Do **not** skip ahead (later steps assume earlier structure)

If you want, next I can:

* Collapse these into **one mega-Claude “Program Manager” prompt**
* Write a **Claude Code agent manifest** for reuse
* Generate a **checklist + acceptance criteria** per phase

Tell me how you want to drive Claude next.
