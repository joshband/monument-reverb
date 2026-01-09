# External Paths for Large References

To keep the packaged skill lean, large assets and bulk references are stored outside the skill folder.

## External stash root
- /Users/noisebox/Desktop/skill_external/juce-audio-graphics-architect

## External directories
- assets/source/ (UI RGBA layers, knobs, Midjourney renders)
- references/juce-upstream/ (JUCE examples, headers, docs)
- references/web/ccrma/ (CCRMA PASP HTML copies)
- references/web/spatial/ (ambisonics + IEM HTML copies)
- references/web/dattorro/ (EffectDesignPart1.pdf)

## Notes
- If you move the stash, update this file and the README pointers.
- The skill should reference these paths directly when large assets are needed.
