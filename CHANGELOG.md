# Changelog

## Unreleased
Chambers output mixing now uses constant-power pan weights to keep all eight FDN taps present in mono while preserving stereo width, and Gravity now applies a first-order high-pass inside the feedback loop to contain low-frequency buildup without further darkening the tail. True Freeze mode now holds the FDN state with unity feedback and a soft limiter, bypasses damping/gravity/diffusion, and ramps back to normal operation on release.
