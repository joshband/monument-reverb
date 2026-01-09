# JUCE Animation Notes

## Module setup
- Add the animation module target (typically `juce::juce_animation`) in CMake when available.
- Verify module naming against the JUCE version in use.

## Animator overview
- The animation module provides `Animator` types, `ValueAnimatorBuilder`, and easing helpers.
- For grouped transitions, use `AnimatorSetBuilder` to compose animations.
- Use `VBlankAnimatorUpdater` when you want vsync-driven animation updates.
- Typical workflow: define an animated value (alpha, position, scale), kick an animation with a duration and easing curve, then read the value during `paint()` or layout updates.
- Keep animation updates on the message thread; avoid touching GUI state from the audio thread.

## Easing patterns
- Use easing curves to avoid linear motion: ease-in, ease-out, and in-out variants.
- For a manual fallback, apply easing to a normalized progress value `t` (0..1) and then interpolate.

## Fallback if Animator is unavailable
- Use `juce::ComponentAnimator` for simple component movement and fades.
- Use a `juce::Timer` to tick progress and call `repaint()` for custom animations.

## Implementation reminders
- Keep animation state separate from audio thread state; feed only smoothed or atomic values from the DSP side.
- For audio-reactive transitions, map RMS or peak values to target animation values rather than driving animations directly from audio callbacks.
