# Plugin Architecture (VST3/AU/AUv3)

You understand VST3 and AudioUnit plugin lifecycles deeply.

## AudioProcessor Lifecycle

```
Constructor
    ↓
setPlayConfigDetails() / setBusesLayout()
    ↓
prepareToPlay(sampleRate, blockSize)  ← Allocate here
    ↓
processBlock() called repeatedly      ← Real-time safe only
    ↓
releaseResources()                    ← Deallocate here
    ↓
Destructor
```

## Parameter System (APVTS)

### Declaration
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout createParameters() {
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    
    // Float with skew for frequency
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"frequency", 1},
        "Frequency",
        juce::NormalisableRange<float>(20.0f, 20000.0f, 0.1f, 0.3f),
        1000.0f,
        juce::AudioParameterFloatAttributes().withLabel("Hz")
    ));
    
    // Choice parameter
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"filterType", 1},
        "Filter Type",
        juce::StringArray{"Lowpass", "Highpass", "Bandpass"},
        0
    ));
    
    // Boolean
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"bypass", 1},
        "Bypass",
        false
    ));
    
    return {params.begin(), params.end()};
}
```

### Thread-Safe Access
```cpp
// In processBlock (audio thread)
float freq = apvts.getRawParameterValue("frequency")->load();

// For UI binding
juce::AudioProcessorValueTreeState::SliderAttachment freqAttachment{
    apvts, "frequency", freqSlider
};
```

## State Serialization

```cpp
void getStateInformation(juce::MemoryBlock& destData) override {
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override {
    auto xml = getXmlFromBinary(data, sizeInBytes);
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}
```

## Bus Layouts

```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    // Mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    
    // Input matches output
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}
```

## CMakeLists.txt Template

```cmake
cmake_minimum_required(VERSION 3.22)
project(MyPlugin VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)

add_subdirectory(JUCE)

juce_add_plugin(MyPlugin
    COMPANY_NAME "MyCompany"
    PLUGIN_MANUFACTURER_CODE Myco
    PLUGIN_CODE Mplu
    
    FORMATS AU VST3 AUv3 Standalone
    PRODUCT_NAME "My Plugin"
    
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    
    # AU
    AU_MAIN_TYPE "kAudioUnitType_Effect"
    
    # VST3
    VST3_CATEGORIES "Fx" "Filter"
    
    # AUv3
    HARDENED_RUNTIME_ENABLED TRUE
    APP_SANDBOX_ENABLED TRUE
)

target_sources(MyPlugin PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(MyPlugin PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MyPlugin PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_plugin_client
    juce::juce_audio_processors
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_basics
    juce::juce_gui_extra
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
```

## Format-Specific Notes

### VST3
- Use `VST3_CATEGORIES` for host browser
- State is XML-based via `getStateInformation`
- Latency reported via `getLatencySamples()`

### AU (Audio Unit)
- `AU_MAIN_TYPE` determines category
- Run `auval -v aufx Mplu Myco` to validate
- Cocoa view optional but recommended

### AUv3 (iOS/macOS)
- Requires App Sandbox
- `HARDENED_RUNTIME_ENABLED TRUE`
- Bundle in host app or standalone

## Host Compatibility

### Logic Pro
- Report tail time: `getTailLengthSeconds()`
- Handle all sample rates (don't assume 44100)
- Implement `processBlockBypassed()`

### Pro Tools
- Precise latency reporting critical
- Handle offline bounce: `isNonRealtime()`
- AAX via iLok wrapping (separate)

### Ableton Live
- Smooth bypass transitions
- Variable buffer sizes
- Proper preset management
