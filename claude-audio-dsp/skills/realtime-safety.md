# Real-Time Safety Rules

All audio-thread code must be real-time safe. Violations cause glitches, clicks, and dropouts.

## Hard Rules - NEVER in Audio Thread

### Memory Operations
```cpp
// FORBIDDEN
new, delete, malloc, free
std::vector::push_back, resize
std::string concatenation
std::make_unique, std::make_shared
juce::String operations
```

### Synchronization
```cpp
// FORBIDDEN
std::mutex, std::lock_guard
juce::CriticalSection
std::condition_variable
std::atomic with contention (CAS loops)
```

### System Calls
```cpp
// FORBIDDEN
File I/O (fopen, ofstream)
Console output (std::cout, DBG in release)
Network operations
System time calls (except high-res timer)
```

### Control Flow
```cpp
// FORBIDDEN
Exceptions (throw, try/catch)
Unbounded loops
Deep recursion
Virtual function calls in hot paths
```

## Safe Patterns

### Pre-allocation in prepareToPlay
```cpp
void prepareToPlay(double sampleRate, int maxBlockSize) override {
    // Allocate everything here
    delayBuffer.resize(maxDelaySamples);
    scratchBuffer.setSize(2, maxBlockSize);
    filter.prepare({sampleRate, (uint32)maxBlockSize, 2});
}
```

### Lock-Free Parameter Access
```cpp
// In processBlock - atomic load is safe
float gain = apvts.getRawParameterValue("gain")->load();

// Or use SmoothedValue (pre-configured in prepareToPlay)
float smoothedGain = gainSmoothed.getNextValue();
```

### Thread-Safe Communication

**Audio → UI (metering):**
```cpp
// Audio thread writes
std::atomic<float> levelL{0.0f}, levelR{0.0f};
levelL.store(newLevel, std::memory_order_relaxed);

// UI thread reads
float displayLevel = levelL.load(std::memory_order_relaxed);
```

**UI → Audio (parameter changes):**
```cpp
// Use APVTS - it's already lock-free
apvts.getParameter("cutoff")->setValueNotifyingHost(newValue);
```

**Complex State Changes (presets, IR loading):**
```cpp
// Use lock-free queue or double-buffering
juce::AbstractFifo fifo{64};
std::array<Command, 64> commandBuffer;

// Non-audio thread
if (fifo.getFreeSpace() > 0) {
    int start, size;
    fifo.prepareToWrite(1, start, size);
    commandBuffer[start] = newCommand;
    fifo.finishedWrite(1);
}

// Audio thread
while (fifo.getNumReady() > 0) {
    int start, size;
    fifo.prepareToRead(1, start, size);
    processCommand(commandBuffer[start]);
    fifo.finishedRead(1);
}
```

### Safe IR/Sample Loading
```cpp
class SafeIRLoader : private juce::Thread {
public:
    void loadIR(const juce::File& file) {
        pendingFile = file;
        startThread();
    }
    
    void run() override {
        // Background thread - allocations OK
        auto newIR = loadAndProcess(pendingFile);
        
        // Atomic swap
        juce::SpinLock::ScopedLockType lock(irLock);
        loadedIR = std::move(newIR);
        irReady.store(true);
    }
    
    // Call from audio thread
    bool trySwapIR(juce::AudioBuffer<float>& dest) {
        if (!irReady.load()) return false;
        
        juce::SpinLock::ScopedTryLockType lock(irLock);
        if (lock.isLocked()) {
            std::swap(dest, loadedIR);
            irReady.store(false);
            return true;
        }
        return false;
    }
    
private:
    juce::File pendingFile;
    juce::AudioBuffer<float> loadedIR;
    juce::SpinLock irLock;
    std::atomic<bool> irReady{false};
};
```

## Code Review Checklist

When reviewing DSP code, verify:

1. **No allocations** - Search for `new`, `make_`, `push_back`, `resize`
2. **No locks** - Search for `mutex`, `lock`, `CriticalSection`
3. **No I/O** - Search for `cout`, `DBG`, `File`, `ofstream`
4. **No exceptions** - Search for `throw`, `try`, `catch`
5. **Bounded loops** - All loops have fixed iteration counts
6. **Pre-allocation** - All buffers sized in `prepareToPlay`
7. **Atomic access** - Parameters via `getRawParameterValue()->load()`

## Common Violations & Fixes

| Violation | Fix |
|-----------|-----|
| `juce::String` in processBlock | Use fixed char arrays or pre-built strings |
| `std::vector::push_back` | Pre-allocate, use fixed-size array |
| `DBG()` in release | Wrap in `#if JUCE_DEBUG` |
| `new` for temporary | Pre-allocate in `prepareToPlay` |
| Mutex for param sync | Use `std::atomic` or APVTS |
