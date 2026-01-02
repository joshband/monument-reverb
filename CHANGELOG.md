# Changelog

## Unreleased
Chambers output mixing now uses constant-power pan weights to keep all eight FDN taps present in mono while preserving stereo width, and Gravity now applies a first-order high-pass inside the feedback loop to contain low-frequency buildup without further darkening the tail. True Freeze has been hardened: delay indices lock, all processing is bypassed, and a hard limiter clamps the held state with a ramped return to normal. Bloom now shapes the late-field envelope by blending an exponential decay with a plateau/decay curve, triggered by input transients and bypassed during Freeze for stable holds. Added Warp to morph the FDN feedback matrix between orthogonal topologies with long smoothing and freeze-safe ramping.
