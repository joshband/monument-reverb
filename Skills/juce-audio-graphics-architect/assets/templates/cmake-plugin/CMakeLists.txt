cmake_minimum_required(VERSION 3.15)
project(@PLUGIN_NAME@ VERSION 0.1.0)

# Option A: add JUCE as a subdirectory in this repo.
# add_subdirectory(JUCE)

# Option B: set JUCE_DIR to a local JUCE path and use that.
# add_subdirectory(${JUCE_DIR} JUCE)

juce_add_plugin(@PLUGIN_NAME@
    COMPANY_NAME "YourCompany"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    FORMATS AU VST3
    PRODUCT_NAME "@PLUGIN_NAME@"
)

# Example third-party module integration (optional):
# include(FetchContent)
# FetchContent_Declare(
#     melatonin_blur
#     GIT_REPOSITORY https://github.com/sudara/melatonin_blur.git
#     GIT_TAG main
# )
# FetchContent_MakeAvailable(melatonin_blur)
# target_link_libraries(@PLUGIN_NAME@ PRIVATE melatonin_blur)

target_sources(@PLUGIN_NAME@ PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_features(@PLUGIN_NAME@ PRIVATE cxx_std_17)

target_link_libraries(@PLUGIN_NAME@ PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
    # juce::juce_animation
)

juce_generate_juce_header(@PLUGIN_NAME@)
