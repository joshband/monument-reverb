# Parameter Binding Infrastructure

Auto-binding UI components to APVTS / AUParameterTree.

## The Contract

A UI component receives `paramId`. It does not know about APVTS directly. A binder object wires it.

```
React-JUCE: <Knob paramId="cutoff" />
    ↓
Factory: creates LayeredRGBAKnob with paramId
    ↓
ParameterBinder: wires APVTS ↔ component
    ↓
Component: receives normalized value updates
```

## JUCE / APVTS Binding

### NormalizedValueSink Interface

```cpp
// Any UI component that receives parameter values
class NormalizedValueSink {
public:
    virtual ~NormalizedValueSink() = default;
    virtual void setNormalizedValue(float v) = 0;
};
```

### ParameterBinder.h

```cpp
#pragma once
#include <JuceHeader.h>
#include <memory>

class ParameterBinder {
public:
    ParameterBinder(juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& paramId,
                    NormalizedValueSink& sink);
    ~ParameterBinder();
    
    // Call when user drags control (message thread only)
    void userSetNormalizedValue(float v);

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::String paramId;
    NormalizedValueSink& sink;
    
    std::atomic<float> lastValue{0.0f};
    juce::RangedAudioParameter* param = nullptr;
    
    struct ListenerImpl;
    std::unique_ptr<ListenerImpl> listener;
    
    void pushToUI(float normalized);
};
```

### ParameterBinder.cpp

```cpp
#include "ParameterBinder.h"

struct ParameterBinder::ListenerImpl : juce::AudioProcessorValueTreeState::Listener {
    ListenerImpl(ParameterBinder& o) : owner(o) {}
    
    void parameterChanged(const juce::String&, float newValue) override {
        // May be called from audio thread!
        owner.lastValue.store(newValue, std::memory_order_relaxed);
        owner.pushToUI(newValue);
    }
    
    ParameterBinder& owner;
};

ParameterBinder::ParameterBinder(juce::AudioProcessorValueTreeState& apvtsIn,
                                 const juce::String& paramIdIn,
                                 NormalizedValueSink& sinkIn)
    : apvts(apvtsIn), paramId(paramIdIn), sink(sinkIn)
{
    param = apvts.getParameter(paramId);
    jassert(param != nullptr);
    
    lastValue.store(param->getValue());
    
    listener = std::make_unique<ListenerImpl>(*this);
    apvts.addParameterListener(paramId, listener.get());
    
    // Initialize UI to current param
    pushToUI(lastValue.load());
}

ParameterBinder::~ParameterBinder() {
    if (listener)
        apvts.removeParameterListener(paramId, listener.get());
}

void ParameterBinder::pushToUI(float normalized) {
    // Marshal to message thread
    juce::MessageManager::callAsync([this, normalized] {
        sink.setNormalizedValue(juce::jlimit(0.0f, 1.0f, normalized));
    });
}

void ParameterBinder::userSetNormalizedValue(float v) {
    v = juce::jlimit(0.0f, 1.0f, v);
    if (param == nullptr) return;
    
    // Begin/end gestures for host automation
    param->beginChangeGesture();
    param->setValueNotifyingHost(v);
    param->endChangeGesture();
}
```

## Why This Design Is Correct

- Audio thread → listener callback → stores value → `callAsync` to UI thread
- UI edits use `beginChangeGesture`/`endChangeGesture` (automation-friendly)
- No polling, no timers, no locks
- Thread-safe atomic for value storage

## Using With LayeredRGBAComponent

```cpp
class LayeredRGBAComponent : public juce::Component, 
                              public NormalizedValueSink
{
public:
    LayeredRGBAComponent(const ComponentManifest& manifest,
                         juce::AudioProcessorValueTreeState* apvts,
                         const juce::String& paramId)
        : manifest(manifest)
    {
        if (apvts != nullptr && paramId.isNotEmpty())
            binder = std::make_unique<ParameterBinder>(*apvts, paramId, *this);
    }
    
    void setNormalizedValue(float v) override {
        value = juce::jlimit(0.0f, 1.0f, v);
        repaint();
    }
    
    void userChangedValue(float v) {
        setNormalizedValue(v);
        if (binder) binder->userSetNormalizedValue(v);
    }

private:
    float value = 0.0f;
    std::unique_ptr<ParameterBinder> binder;
};
```

## AUv3 Equivalent (Swift)

```swift
class ParameterBridge: ObservableObject {
    @Published var values: [String: Float] = [:]
    
    private var parameterTree: AUParameterTree?
    private var observerToken: AUParameterObserverToken?
    
    func bind(paramId: String) -> Binding<Double> {
        Binding(
            get: { Double(self.values[paramId] ?? 0) },
            set: { self.setParameter(paramId, value: Float($0)) }
        )
    }
    
    private func setParameter(_ id: String, value: Float) {
        guard let param = parameterTree?.parameter(withAddress: addressFor(id)) else { return }
        param.setValue(value, originator: observerToken)
    }
}
```

## Factory Integration

```cpp
class VisualDNAComponentFactory {
public:
    explicit VisualDNAComponentFactory(juce::AudioProcessorValueTreeState& apvts)
        : apvts(apvts) {}
    
    std::unique_ptr<juce::Component> create(const juce::var& props) {
        auto componentId = props["component"].toString();
        auto paramId = props["paramId"].toString();
        
        auto manifest = ManifestLoader::load(componentId);
        return std::make_unique<LayeredRGBAComponent>(manifest, &apvts, paramId);
    }

private:
    juce::AudioProcessorValueTreeState& apvts;
};
```

## Thread Safety Summary

| Operation | Thread | Safe? |
|-----------|--------|-------|
| `parameterChanged` callback | Audio/arbitrary | ✅ Uses atomic + callAsync |
| `setNormalizedValue` | Message | ✅ UI update |
| `userSetNormalizedValue` | Message | ✅ Notifies host |
| `repaint()` | Message | ✅ Normal JUCE |
