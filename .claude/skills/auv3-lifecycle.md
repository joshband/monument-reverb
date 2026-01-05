# AUv3 Lifecycle & Threading

You understand AUv3 lifecycle and threading rules in detail.

## Lifecycle Facts

```
AUAudioUnit instantiated
    ↓
allocateRenderResources()  ← May be called multiple times
    ↓
renderBlock called repeatedly (real-time thread)
    ↓
deallocateRenderResources()  ← Must fully release state
    ↓
UI may be created/destroyed independently
```

- `AUAudioUnit` is instantiated before UI
- UI may be created and destroyed multiple times
- `renderBlock` runs on the real-time audio thread
- `allocateRenderResources()` may be called repeatedly
- `deallocateRenderResources()` must fully release DSP state

## Threading Rules

| Method | Thread | Safe Operations |
|--------|--------|-----------------|
| `renderBlock` | Real-time audio | NO alloc, NO locks, NO ObjC messaging |
| `allocateRenderResources` | Main/arbitrary | Allocations OK |
| `deallocateRenderResources` | Main/arbitrary | Cleanup OK |
| Parameter observers | NOT guaranteed | Dispatch to main for UI |
| UI methods | Main thread | Normal Cocoa/SwiftUI |

## renderBlock Requirements

```objc
// CORRECT: Capture only POD or preallocated state
__block float* buffer = preAllocatedBuffer;
__block DSPProcessor* dsp = &dspProcessor;

self.internalRenderBlock = ^AUAudioUnitStatus(
    AudioUnitRenderActionFlags *actionFlags,
    const AudioTimeStamp *timestamp,
    AUAudioFrameCount frameCount,
    NSInteger outputBusNumber,
    AudioBufferList *outputData,
    const AURenderEvent *realtimeEventListHead,
    AURenderPullInputBlock pullInputBlock)
{
    // NO Objective-C messaging here
    // NO [self anything]
    // NO NSLog, NSString, etc.
    
    dsp->process(outputData, frameCount);
    return noErr;
};
```

## Parameter Changes (Lock-Free)

```objc
// Audio thread reads atomically
float value = parameterTree.parameterWithAddress(addr).value;

// UI thread sets (thread-safe)
[parameter setValue:newValue];

// Observer callback - may be off main thread!
parameter.implementorValueObserver = ^(AUParameter *param, AUValue value) {
    // Dispatch to main for UI updates
    dispatch_async(dispatch_get_main_queue(), ^{
        [self updateUIForParameter:param value:value];
    });
};
```

## State Restoration (Idempotent)

```objc
- (void)setFullState:(NSDictionary *)fullState {
    // Must handle:
    // - Missing keys
    // - Wrong types
    // - Called before allocateRenderResources
    // - Called multiple times
    
    if (fullState[@"version"]) {
        // Migrate if needed
    }
    
    // Apply to parameter tree (thread-safe)
    for (AUParameter *param in _parameterTree.allParameters) {
        NSNumber *value = fullState[param.identifier];
        if (value) {
            param.value = value.floatValue;
        }
    }
}
```

## Code Review Checklist

When reviewing AUv3 code:
- [ ] No Objective-C allocations inside `renderBlock`
- [ ] No `dispatch_sync` to main from audio thread
- [ ] `renderBlock` captures only POD or preallocated state
- [ ] Parameter observers dispatch UI updates to main
- [ ] `deallocateRenderResources` releases all DSP state
- [ ] State restoration is idempotent
