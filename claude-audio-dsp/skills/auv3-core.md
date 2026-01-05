# AUv3 Core Knowledge

You are operating as a professional AUv3 (Audio Unit v3) plugin engineer.

**AUv3 is NOT equivalent to desktop AU/VST3.**

## Hard Constraints

- Runs inside a sandboxed app extension
- Shared memory and IPC are restricted
- UI and DSP live in separate processes
- State must be serializable and deterministic
- Cold-start time matters (App Store review!)

## Assumptions

- iOS / iPadOS target unless explicitly stated
- Audio is pull-based via `renderBlock`
- Real-time constraints are stricter than desktop

## Architecture

```
Host App
└── AUv3 Extension (sandboxed)
    ├── AUAudioUnit (DSP)
    │   ├── AUParameterTree
    │   └── renderBlock (real-time)
    └── AUAudioUnitViewController (UI, separate process)
```

## Key Differences from Desktop

| Desktop AU/VST3 | AUv3 |
|-----------------|------|
| Single process | Extension + host processes |
| Filesystem access | Sandboxed, App Group only |
| UI always available | UI may not exist |
| Generous memory | Memory pressure common |
| Startup time flexible | Must be fast |

## Code Organization

```
MyAUv3/
├── Shared/
│   └── DSP/           # Platform-agnostic C++
├── AUv3Extension/
│   ├── AUv3AudioUnit.mm
│   ├── AUv3ViewController.swift
│   └── Info.plist
└── HostApp/
    └── (minimal container)
```

## When Generating AUv3 Code

1. Clearly separate DSP, parameter model, and UI
2. Call out which thread each method runs on
3. Avoid desktop-only assumptions
4. DSP core must be platform-agnostic C++
5. Objective-C++ only as thin bridge
