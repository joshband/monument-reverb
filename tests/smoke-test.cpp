#include <JuceHeader.h>

#include "plugin/PluginProcessor.h"

int main()
{
    juce::ScopedJuceInitialiser_GUI juceInit;

    MonumentAudioProcessor processor;
    processor.prepareToPlay(44100.0, 512);

    juce::AudioBuffer<float> buffer(2, 512);
    buffer.clear();

    juce::MidiBuffer midi;
    processor.processBlock(buffer, midi);
    processor.releaseResources();

    return 0;
}
