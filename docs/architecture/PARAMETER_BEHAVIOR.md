# Monument Parameter Behavior Contract

This document defines how each parameter maps to DSP behavior. All parameters are normalized to [0, 1] unless noted. All mappings must be smooth (at least 20 ms ramp) and must never allocate or lock on the audio thread.

## Mass

- Perceptual meaning
  - Perceived material weight of the space: heavier = darker, denser tail with slower high-frequency decay.
- Technical impact
  - In Chambers, increase low-pass damping and reduce high-frequency feedback energy.
  - In Buttress, apply gentler saturation curve to prevent brittle highs when mass is high.
- Interactions
  - With Time: high Mass + long Time should preserve low-end while rolling off highs faster.
  - With Air: Air can counteract Mass by restoring upper-band energy post-FDN.
- Guardrails
  - Clamp damping to a stable range (e.g., 0.05–0.95 coefficient).
  - Enforce minimum bandwidth to avoid total HF collapse.

## Time

- Perceptual meaning
  - Global decay length of the late field.
- Technical impact
  - In Chambers, set FDN feedback gain and optionally scale delay lengths.
  - In Buttress, adjust feedback limiter threshold to maintain stability at long decay times.
- Interactions
  - With Density: higher density at longer times increases perceived size; avoid over-smearing.
  - With Freeze: when Freeze is active, Time should not affect feedback gain.
- Guardrails
  - Clamp feedback below unity (e.g., <= 0.95) unless Freeze overrides.
  - Apply smoothing to avoid clicks during time changes.

## Density

- Perceptual meaning
  - Reflective complexity: low = discrete echoes, high = continuous diffusion.
- Technical impact
  - In Pillars, increase multi-tap gain and diffusion tap count/spacing.
  - In Chambers, increase input diffusion and pre-FDN mixing matrix spread.
- Interactions
  - With Bloom: higher Density makes Bloom feel more continuous.
  - With Drift: high Density + high Drift can blur transients excessively.
- Guardrails
  - Maintain bounded energy by normalizing tap gains.
  - Ensure diffusion stages remain stable at max Density.

## Bloom

- Perceptual meaning
  - How quickly the tail swells after the initial reflections.
- Technical impact
  - In Chambers, apply a slow gain envelope to late field injection or feedback.
  - In Pillars, reduce early reflection dominance at higher Bloom.
- Interactions
  - With Time: longer Time + high Bloom yields larger tail buildup.
  - With Gravity: strong Gravity can counteract Bloom in higher bands.
- Guardrails
  - Clamp bloom gain to prevent runaway feedback.
  - Smooth envelope changes over multiple blocks.

## Air

- Perceptual meaning
  - Upper-band openness and shimmer in the tail.
- Technical impact
  - In Facade, apply high-shelf boost or attenuation.
  - In Chambers, optionally lift high-band diffusion to keep brightness in late tail.
- Interactions
  - With Mass: Air counterbalances Mass darkening.
  - With Drift/Warp: high Air can exaggerate modulation artifacts.
- Guardrails
  - Limit boost to prevent clipping; apply output gain compensation.
  - Maintain stable HF damping to avoid ringing.

## Width

- Perceptual meaning
  - Stereo spread of the reverb field.
- Technical impact
  - In Facade, apply mid/side width scaling and optional decorrelation.
- Interactions
  - With Density: high Density and high Width can smear center image.
  - With Mix: extreme Width at low Mix can feel phasey; keep dry center stable.
- Guardrails
  - Clamp width to avoid negative mono compatibility (e.g., 0.0–2.0).
  - Apply a gentle mono fold-down safety when Width is extreme.

## Mix (0–1 mapped from 0–100%)

- Perceptual meaning
  - Wet/dry blend.
- Technical impact
  - Constant-power crossfade between dry and wet signals.
- Interactions
  - With Width: apply width only to wet signal; preserve dry center.
  - With Freeze: Freeze affects wet path only.
- Guardrails
  - Clamp to [0, 1] and use equal-power curves to avoid level dips.
  - Avoid reallocations when switching Mix extremes.

## Warp

- Perceptual meaning
  - Space distortion: pitch smear, non-linear delay perturbation.
- Technical impact
  - In Weathering/Chambers, modulate delay taps with non-linear offsets.
  - Optional micro pitch shift via interpolated delay modulation.
- Interactions
  - With Drift: Drift adds random motion; Warp adds structured distortion.
  - With Time: long Time amplifies Warp artifacts.
- Guardrails
  - Limit modulation depth to avoid zippering and aliasing.
  - Use smooth interpolation and restrict rate changes.

## Drift

- Perceptual meaning
  - Slow, random motion in the reverb field.
- Technical impact
  - In Weathering, apply low-rate random walk to modulation phase and delay lengths.
- Interactions
  - With Density: high Density + high Drift yields a fog-like tail.
  - With Air: Drift can smear high frequencies; reduce Air boost if needed.
- Guardrails
  - Constrain random walk rate and depth to prevent pitch instability.
  - Keep modulation under 1–2% of delay length.

## Gravity

- Perceptual meaning
  - Spectral tilt of decay: heavier lows, faster high decay.
- Technical impact
  - In Chambers, apply frequency-dependent decay (low band longer, high band shorter).
  - In Facade, apply a gentle low-shelf if needed to reinforce weight.
- Interactions
  - With Mass: Gravity + Mass can over-darken; Air can counterbalance.
  - With Bloom: strong Gravity can reduce perceived bloom brightness.
- Guardrails
  - Clamp low-band feedback to avoid LF runaway.
  - Maintain a minimum high-band decay for articulation.

## Freeze

- Perceptual meaning
  - Infinite hold of the current space state.
- Technical impact
  - In Chambers, set feedback to unity minus safety margin and mute input injection.
  - In Buttress, engage strict limiter/saturation to prevent energy accumulation.
- Interactions
  - With Time: Time ignored or used only to set pre-freeze decay.
  - With Mix: wet path frozen; dry path remains unaffected.
- Guardrails
  - Apply hard cap on feedback (e.g., 0.995) and enforce limiter.
  - High-pass or DC blocker to prevent low-frequency buildup.
  - Smooth transitions into and out of Freeze to avoid bursts.
