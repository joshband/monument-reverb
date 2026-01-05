# Claude Audio DSP Plugin

Professional audio plugin development toolkit for Claude Code.

## Purpose

Accelerate VST3/AU/AUv3 plugin development with:
- DSP and real-time constraints as first-class rules
- Specialized agents for DSP, plugin formats, and UI
- Safe, idiomatic JUCE code generation
- Photorealistic UI component systems
- Benchmarking and testing harnesses

## Quick Start

```bash
# Clone to your project
cp -r claude-audio-dsp/.claude your-project/
cp -r claude-audio-dsp/skills your-project/.claude/
cp -r claude-audio-dsp/agents your-project/.claude/
cp -r claude-audio-dsp/commands your-project/.claude/
```

## Commands

| Command | Description |
|---------|-------------|
| `/gen-plugin <name>` | Generate complete VST3/AU plugin project |
| `/gen-dsp <algorithm>` | Generate DSP algorithm with math + C++ |
| `/bench-dsp <class>` | Generate CPU benchmarking harness |
| `/gen-tests <class>` | Generate audio test suite |
| `/gen-rgba-component-pack <type>` | Generate layered UI component pack |
| `/review-dsp` | Review code for real-time safety |

## Skills (Always-On Knowledge)

- **dsp-core** - DSP fundamentals, filters, dynamics
- **realtime-safety** - Thread safety, lock-free patterns
- **plugin-architecture** - VST3/AU lifecycle, APVTS
- **juce-modules** - juce::dsp, MIDI, utilities
- **photorealistic-ui** - Layered sprites, shadows, animation
- **auv3-core** - AUv3 constraints and architecture
- **auv3-lifecycle** - Threading, renderBlock rules
- **auv3-ui-parameters** - AUParameterTree, SwiftUI binding
- **auv3-packaging** - App extensions, entitlements

## Agents (Specialized Roles)

- **dsp-expert** - Algorithm design and optimization
- **juce-vst3-au** - Plugin scaffolding and CMake
- **juce-ui-controls** - APVTS bindings, custom controls
- **dsp-reviewer** - Real-time safety audits
- **simd-optimizer** - Vectorization guidance
- **auv3-swiftui-ui** - SwiftUI AUv3 interfaces
- **swiftui-knob-canvas** - High-performance Canvas knobs
- **photoreal-rgba-components** - Layered asset systems

## Directory Structure

```
claude-audio-dsp/
├── .claude/
│   └── settings.json
├── CLAUDE.md              # Main instructions
├── skills/                # Always-on knowledge
├── agents/                # Specialized personas
├── commands/              # Slash commands
├── hooks/                 # Automation
├── schemas/               # JSON schemas
├── tools/                 # Validator scripts
└── README.md
```

## Real-Time Safety

The plugin enforces these rules in all generated code:

**Never in processBlock:**
- Memory allocation
- Locks/mutexes
- System calls
- Exceptions
- Unbounded loops

**Always:**
- Pre-allocate in `prepareToPlay()`
- Use `SmoothedValue` for parameters
- Use `ScopedNoDenormals`
- Access params via APVTS atomics

## Asset Pipeline (Photorealistic UI)

```
Blender/Substance (3D render)
    ↓
Export layered PNG (shadow, base, spec, indicator...)
    ↓
component.manifest.json (layer stack + states)
    ↓
Validate: python tools/validate_component_pack.py Pack/
    ↓
Runtime compositing (SwiftUI/JUCE/Web)
```

## License

MIT - Use freely in commercial projects.

---

*This is a productivity amplifier, not a replacement for listening tests. Trust your ears.*
