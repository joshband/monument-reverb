# Advanced Feature Scaffolding

Design notes and integration ideas for future experimental modules.

## Multiple Algorithms

Goal: swap between different FDN/diffusion configurations ("Nebula", "Granite").

- Add an enum or parameter to select the algorithm.
- Each algorithm provides its own delay lengths, feedback matrix, and diffusion settings.
- Integration point: `Chambers::prepare()` selects a topology preset and precomputes matrices.

## Colour Modes

Goal: apply global tone filters (Bright/Neutral/Dark) to the late field.

- Add a lightweight tilt EQ after the FDN but before Facade.
- Parameter: `colourMode` (0-2) or continuous tilt amount.
- Integration point: a new module between Chambers and Facade or inside Facade.

## Reverse or Pitch-Shifted Pillars

Goal: reverse early taps or add shimmer-like octave feedback.

- Reverse: store a short buffer per tap and read it backwards during the early stage.
- Shimmer: add a subtle octave-up path to Pillars only (keeps FDN clean).
- Integration point: Pillars before Chambers. Keep it optional and off by default.

## Tape Loop Module

Goal: parallel circular buffer that records wet output and feeds back with low-pass, saturation, and flutter.

- Capture wet signal post-Buttress, feed back into the loop with mild saturation.
- Add slow wow/flutter LFO for pitch smear, low-pass per pass for decay.
- Integration point: between Buttress and Facade, mixed with main wet output.

## Sound-on-Sound Mode

Goal: stack multiple freezes or drift loops with optional half-speed or reverse playback.

- Maintain a small pool of frozen buffers (2-4) with independent read heads.
- Allow half-speed playback and reverse as optional variants.
- Integration point: after Chambers, before Buttress (so safety limiting still applies).
