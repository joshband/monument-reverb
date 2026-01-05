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

---

## Physical Modeling Parameters (Phase 5)

### TubeRayTracer Module

## Tube Count

- Perceptual meaning
  - Number of interconnected metal tubes in the network (1-8 tubes).
- Technical impact
  - In TubeRayTracer, determines the complexity of the tube network topology.
  - More tubes create richer harmonic content and increased resonance density.
  - Each tube adds computational cost and potential coupling points.
- Interactions
  - With Metallic Resonance: more tubes amplify the metallic ringing effect.
  - With Coupling Strength: higher tube count increases the number of interaction pathways.
  - With Radius Variation: diverse tube sizes create complex harmonic relationships.
- Guardrails
  - Clamp tube count to valid range [1, 8] to maintain performance.
  - Scale processing load proportionally with tube count.
  - Normalize energy across tubes to prevent accumulation.

## Tube Radius Variation

- Perceptual meaning
  - Spread of tube diameters creating pitch diversity in the network.
- Technical impact
  - In TubeRayTracer, controls the uniformity vs diversity of tube resonant frequencies.
  - Low values (0.0-0.3): uniform pipes with consistent pitch and focused resonance.
  - High values (0.7-1.0): diverse diameters creating complex, inharmonic tuning.
- Interactions
  - With Tube Count: variation matters more with higher tube counts.
  - With Metallic Resonance: diverse tubes create more complex spectral coloration.
  - With Material macro: Material controls uniformity (inverse relationship).
- Guardrails
  - Maintain minimum tube radius to avoid unstable resonances.
  - Ensure radius values stay within physically plausible ranges.
  - Apply smoothing to prevent abrupt pitch shifts.

## Metallic Resonance

- Perceptual meaning
  - Surface reflectivity and brightness of metal ringing.
- Technical impact
  - In TubeRayTracer, controls the feedback gain and high-frequency content of tube resonances.
  - Low values (0.0-0.3): damped, matte metal surfaces with minimal ringing.
  - High values (0.7-1.0): highly reflective surfaces with bright, sustained harmonic content.
- Interactions
  - With Tube Count: resonance multiplies across all tubes.
  - With Material macro: Material directly drives metallic character (0.3→0.9).
  - With Air: Air can enhance or counteract the brightness of metallic resonance.
- Guardrails
  - Clamp resonance feedback below unity to prevent runaway buildup.
  - Apply gentle high-frequency roll-off to avoid harshness at extreme values.
  - Use smoothing to prevent zipper noise during parameter changes.

## Tube Coupling Strength

- Perceptual meaning
  - Energy transfer intensity between interconnected tubes.
- Technical impact
  - In TubeRayTracer, controls how strongly tubes influence each other at connection points.
  - Low values (0.0-0.3): isolated tubes with discrete, separate resonances.
  - High values (0.7-1.0): strong interaction creating blended, complex harmonics.
- Interactions
  - With Tube Count: coupling creates exponential complexity with more tubes.
  - With Topology macro: Topology controls coupling strength (0.2→0.8).
  - With Metallic Resonance: coupling spreads resonance energy across the network.
- Guardrails
  - Normalize energy transfer to prevent accumulation at coupling points.
  - Limit coupling to maintain stability in feedback paths.
  - Apply smoothing to prevent sudden energy redistributions.

### ElasticHallway Module

## Wall Elasticity

- Perceptual meaning
  - Maximum deformation amount of walls under acoustic pressure.
- Technical impact
  - In ElasticHallway, controls how much walls physically bend in response to sound.
  - Low values (0.0-0.3): rigid walls with minimal movement.
  - High values (0.7-1.0): highly elastic walls with large deformations.
  - Wall displacement modulates reflection delays and absorption.
- Interactions
  - With Recovery Time: elasticity determines peak displacement before recovery begins.
  - With Elasticity macro: Elasticity macro is the primary control (0.1→0.9).
  - With Density: elastic walls add organic motion to dense reverb fields.
- Guardrails
  - Limit maximum displacement to prevent unrealistic wall positions.
  - Ensure wall motion stays within bounded physical range.
  - Apply smoothing to prevent abrupt position changes.

## Recovery Time

- Perceptual meaning
  - Wall return-to-rest speed after deformation.
- Technical impact
  - In ElasticHallway, controls the time constant for spring-like wall recovery.
  - Low values (0.0-0.3): fast snap-back (20-50ms), responsive walls.
  - High values (0.7-1.0): slow recovery (500-1000ms), sustained deformation.
  - Affects the temporal envelope of wall-modulated reflections.
- Interactions
  - With Wall Elasticity: recovery time shapes the deformation envelope curve.
  - With Viscosity macro: Viscosity controls recovery time (0.2→0.8).
  - With Evolution: slow recovery creates evolving acoustic character.
- Guardrails
  - Clamp recovery time to reasonable physical range (10ms-2000ms).
  - Apply smoothing to prevent discontinuities in wall motion.
  - Ensure stability in spring dynamics at extreme values.

## Absorption Drift

- Perceptual meaning
  - Rate of slow material property evolution over time.
- Technical impact
  - In ElasticHallway, modulates the absorption coefficient with slow random walk.
  - Low values (0.0-0.3): static absorption, consistent material character.
  - High values (0.7-1.0): continuously morphing surface properties.
  - Creates slow timbral evolution in the acoustic space.
- Interactions
  - With Evolution macro: Evolution controls drift rate (0.0→0.6).
  - With Mass: drift can modulate the perceived weight and damping.
  - With Drift (core parameter): both create slow motion but at different rates.
- Guardrails
  - Constrain drift to prevent extreme absorption values (keep 0.1-0.9).
  - Use smooth random walk to avoid sudden spectral changes.
  - Limit modulation depth to maintain acoustic coherence.

## Elastic Nonlinearity

- Perceptual meaning
  - Deformation curve shape (linear vs exponential response).
- Technical impact
  - In ElasticHallway, controls the relationship between pressure and wall displacement.
  - Low values (0.0-0.3): linear spring behavior, proportional response.
  - High values (0.7-1.0): exponential curve, dramatic response to loud signals.
  - Nonlinearity creates dynamic-dependent acoustic character.
- Interactions
  - With Wall Elasticity: nonlinearity shapes how elasticity responds to input level.
  - With Chaos macro: Chaos controls nonlinearity (0.1→0.9).
  - With Audio Follower modulation: nonlinearity enhances dynamic responsiveness.
- Guardrails
  - Ensure nonlinear curve remains stable at all input levels.
  - Apply gentle limiting to prevent extreme displacement values.
  - Maintain smooth transitions across the nonlinear curve.

### AlienAmplification Module

## Impossibility Degree

- Perceptual meaning
  - Amount of physics violation and non-Euclidean acoustic behavior.
- Technical impact
  - In AlienAmplification, controls the intensity of impossible acoustic effects.
  - Low values (0.0-0.3): subtle bending of acoustic rules.
  - High values (0.7-1.0): extreme impossible behaviors (energy gain, topology folding).
  - Modulates gain, spectral morphing, and spatial impossibility.
- Interactions
  - With Chaos macro: Chaos drives impossibility degree (0.0→0.7).
  - With Paradox Gain: impossibility enables energy amplification.
  - With Buttress: Buttress must protect against impossible energy accumulation.
- Guardrails
  - Apply safety limiting to prevent runaway energy from impossible reflections.
  - Clamp output gain to maintain headroom despite amplification.
  - Use smoothing to prevent sudden impossibility shifts.

## Pitch Evolution

- Perceptual meaning
  - Slow harmonic drift rate creating spectral morphing.
- Technical impact
  - In AlienAmplification, modulates the harmonic content over time.
  - Low values (0.0-0.3): stable pitch and harmonic structure.
  - High values (0.7-1.0): continuous slow pitch shifting and spectral evolution.
  - Creates alien, morphing timbres over long timescales.
- Interactions
  - With Evolution macro: Evolution controls pitch evolution (0.0→0.5).
  - With Time: longer reverb tails amplify the pitch evolution effect.
  - With Impossibility: pitch evolution is more extreme at high impossibility.
- Guardrails
  - Limit pitch shift depth to avoid complete loss of harmonic relationship.
  - Apply smooth interpolation to prevent glitching or aliasing.
  - Use gentle modulation rates to maintain musicality.

## Paradox Frequency

- Perceptual meaning
  - Rate of acoustic topology folding events.
- Technical impact
  - In AlienAmplification, controls how often impossible topology shifts occur.
  - Low values (0.0-0.3): rare events (0.1-0.5 Hz), occasional surprises.
  - High values (0.7-1.0): frequent folding (2-5 Hz), continuous impossibility.
  - Triggers periodic spatial reconfigurations and energy redistributions.
- Interactions
  - With Paradox Gain: frequency determines how often gain events occur.
  - With Warp: paradox frequency creates rhythmic impossibility vs continuous warp.
  - With modulation sources: can be modulated for evolving impossibility patterns.
- Guardrails
  - Limit frequency to prevent rapid switching that could cause zipper noise.
  - Apply crossfading during topology shifts to maintain smoothness.
  - Ensure topology changes don't create phase cancellation artifacts.

## Paradox Gain

- Perceptual meaning
  - Energy amplification intensity during impossible reflection events.
- Technical impact
  - In AlienAmplification, controls the gain boost during paradox events.
  - Low values (0.0-0.3): subtle energy gain, barely perceptible boosts.
  - High values (0.7-1.0): strong amplification, dramatic energy increases.
  - Violates energy conservation for surreal, impossible acoustics.
- Interactions
  - With Impossibility Degree: gain is scaled by impossibility amount.
  - With Paradox Frequency: frequency determines how often gain occurs.
  - With Buttress: critical safety limiting required to prevent clipping.
- Guardrails
  - Apply strict output limiting to prevent clipping from energy gain.
  - Use smooth gain ramping to avoid sudden level jumps.
  - Monitor accumulated energy and apply makeup attenuation if needed.
  - High-pass filter to prevent low-frequency energy buildup.

---

## Modulation System Parameters

### Connection Probability (Per-Connection)

- Perceptual meaning
  - Likelihood that a modulation connection is active during any given block.
  - Creates intermittent, evolving modulation patterns.
- Technical impact
  - In ModulationMatrix, each connection evaluated per audio block.
  - 0%: Connection never applies modulation (effectively bypassed).
  - 50%: Modulation active approximately 50% of the time (unpredictable gating).
  - 100%: Always active (traditional continuous modulation).
  - Block-rate evaluation: probability check once per buffer, not per sample.
- Interactions
  - With Depth: probability gates the entire modulation amount (depth × probability).
  - With Smoothing: smoothing still applies when connection is active, preventing zipper noise.
  - With multiple connections: each connection evaluated independently.
- Guardrails
  - Probability clamped to [0.0, 1.0] range (0-100%).
  - RNG seeded per-block for deterministic behavior within a block.
  - No additional smoothing on probability transitions (intentional discontinuity).
- Musical use cases
  - Low probability (10-30%): Occasional surprises, sparse texture variation.
  - Medium probability (40-60%): Rhythmic gating, pulsing modulation.
  - High probability (70-90%): Mostly-on with occasional dropouts, breathing effect.
  - Combined with multiple connections: layered intermittent patterns create evolving soundscapes.
