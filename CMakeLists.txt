cmake_minimum_required(VERSION 3.21)

project(Monument VERSION 0.1.0 LANGUAGES CXX C)

if(APPLE)
  if(NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for macOS" FORCE)
  endif()
  if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "12.0" CACHE STRING "Minimum macOS deployment target")
  endif()
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(MONUMENT_USE_LOCAL_JUCE "Use a local JUCE checkout instead of FetchContent" OFF)
option(MONUMENT_COPY_PLUGIN_AFTER_BUILD "Copy plugin to system plugin folders after build" ON)

if(MONUMENT_USE_LOCAL_JUCE)
  if(NOT DEFINED JUCE_SOURCE_DIR)
    message(FATAL_ERROR "MONUMENT_USE_LOCAL_JUCE is ON but JUCE_SOURCE_DIR is not set.")
  endif()
  add_subdirectory(${JUCE_SOURCE_DIR} JUCE)
else()
  include(FetchContent)
  FetchContent_Declare(
    juce
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.12
  )
  FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(Monument
  COMPANY_NAME "Monument Audio"
  BUNDLE_ID "com.monumentaudio.monument"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD ${MONUMENT_COPY_PLUGIN_AFTER_BUILD}
  PLUGIN_MANUFACTURER_CODE Mnmt
  PLUGIN_CODE Mnmt
  FORMATS AU VST3 Standalone
  PRODUCT_NAME "Monument"
)

juce_generate_juce_header(Monument)

target_sources(Monument
  PRIVATE
    plugin/PluginProcessor.h
    plugin/PluginProcessor.cpp
    plugin/PresetManager.h
    plugin/PresetManager.cpp
    plugin/PluginEditor.h
    plugin/PluginEditor.cpp
    plugin/PluginEditorV2.h
    plugin/PluginEditorV2.cpp
    dsp/DspModule.h
    dsp/DspModules.h
    dsp/DspModules.cpp
    dsp/AllpassDiffuser.h
    dsp/AllpassDiffuser.cpp
    dsp/MemoryEchoes.h
    dsp/MemoryEchoes.cpp
    dsp/ParameterSmoother.h
    dsp/ParameterSmoother.cpp
    dsp/Chambers.h
    dsp/Chambers.cpp
    dsp/SpatialProcessor.h
    dsp/SpatialProcessor.cpp
    dsp/MacroMapper.h
    dsp/MacroMapper.cpp
    dsp/ExpressiveMacroMapper.h
    dsp/ExpressiveMacroMapper.cpp
    dsp/ModulationMatrix.h
    dsp/ModulationMatrix.cpp
    dsp/SequenceScheduler.h
    dsp/SequenceScheduler.cpp
    dsp/SequencePresets.h
    dsp/SequencePresets.cpp
    dsp/TubeRayTracer.h
    dsp/TubeRayTracer.cpp
    dsp/ElasticHallway.h
    dsp/ElasticHallway.cpp
    dsp/AlienAmplification.h
    dsp/AlienAmplification.cpp
    dsp/DspRoutingGraph.h
    dsp/DspRoutingGraph.cpp
    dsp/ExperimentalModulation.h
    dsp/ExperimentalModulation.cpp
    # UI components will be added here as needed
)

# Add sprite sheet assets as embedded BinaryData
# Using HEADER_NAME and NAMESPACE to make it accessible
# Binary assets disabled for simple JUCE UI approach
# Re-enable if custom assets are needed
# juce_add_binary_data(MonumentAssets
#   HEADER_NAME BinaryData.h
#   NAMESPACE BinaryData
#   SOURCES
#     # Add assets here as needed
# )

# Define DSP sources for tests that need full plugin functionality
set(DSP_SOURCES
  dsp/DspModule.h
  dsp/DspModules.h
  dsp/DspModules.cpp
  dsp/AllpassDiffuser.h
  dsp/AllpassDiffuser.cpp
  dsp/MemoryEchoes.h
  dsp/MemoryEchoes.cpp
  dsp/ParameterSmoother.h
  dsp/ParameterSmoother.cpp
  dsp/Chambers.h
  dsp/Chambers.cpp
  dsp/SpatialProcessor.h
  dsp/SpatialProcessor.cpp
  dsp/MacroMapper.h
  dsp/MacroMapper.cpp
  dsp/ExpressiveMacroMapper.h
  dsp/ExpressiveMacroMapper.cpp
  dsp/ModulationMatrix.h
  dsp/ModulationMatrix.cpp
  dsp/SequenceScheduler.h
  dsp/SequenceScheduler.cpp
  dsp/SequencePresets.h
  dsp/SequencePresets.cpp
  dsp/TubeRayTracer.h
  dsp/TubeRayTracer.cpp
  dsp/ElasticHallway.h
  dsp/ElasticHallway.cpp
  dsp/AlienAmplification.h
  dsp/AlienAmplification.cpp
  dsp/DspRoutingGraph.h
  dsp/DspRoutingGraph.cpp
  dsp/ExperimentalModulation.h
  dsp/ExperimentalModulation.cpp
)

target_include_directories(Monument
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(Monument
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    MONUMENT_TESTING_UI=1
)

target_link_libraries(Monument
  PRIVATE
    # MonumentAssets # Disabled for simple JUCE UI
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_opengl
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

juce_add_gui_app(monument_ui_prototype
  PRODUCT_NAME "Monument UI Prototype"
  COMPANY_NAME "Monument Audio"
  BUNDLE_ID "com.monumentaudio.monument.ui.prototype"
)

juce_generate_juce_header(monument_ui_prototype)

target_sources(monument_ui_prototype
  PRIVATE
    ui/MonumentUiPrototype.cpp
)

target_include_directories(monument_ui_prototype
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_compile_definitions(monument_ui_prototype
  PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    MONUMENT_TESTING_UI=1
)

target_link_libraries(monument_ui_prototype
  PRIVATE
    Monument
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_gui_extra
    juce::juce_opengl
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

option(MONUMENT_ENABLE_TESTS "Build Monument legacy tests (DEPRECATED for DSP authority)" OFF)
option(MONUMENT_ENABLE_MEMORY "Enable Memory Echoes in plugin" OFF)
option(MONUMENT_LEGACY_UI "Use legacy UI layout" OFF)

if(MONUMENT_ENABLE_MEMORY)
  target_compile_definitions(Monument
    PRIVATE
      MONUMENT_ENABLE_MEMORY=1
  )
endif()

if(MONUMENT_LEGACY_UI)
  target_compile_definitions(Monument
    PRIVATE
      MONUMENT_LEGACY_UI=1
  )
endif()

if(MONUMENT_ENABLE_TESTS)
  enable_testing()
  juce_add_console_app(monument_smoke_test
    PRODUCT_NAME "Monument Smoke Test"
  )
  juce_generate_juce_header(monument_smoke_test)
  target_sources(monument_smoke_test
    PRIVATE
      tests/smoke-test.cpp
      plugin/PluginProcessor.h
      plugin/PluginProcessor.cpp
      plugin/PresetManager.h
      plugin/PresetManager.cpp
      dsp/DspModule.h
      dsp/DspModules.h
      dsp/DspModules.cpp
      dsp/DspRoutingGraph.h
      dsp/DspRoutingGraph.cpp
      dsp/AllpassDiffuser.h
      dsp/AllpassDiffuser.cpp
      dsp/MemoryEchoes.h
      dsp/MemoryEchoes.cpp
      dsp/ParameterSmoother.h
      dsp/ParameterSmoother.cpp
      dsp/Chambers.h
      dsp/Chambers.cpp
      dsp/SpatialProcessor.h
      dsp/SpatialProcessor.cpp
      dsp/MacroMapper.h
      dsp/MacroMapper.cpp
      dsp/ModulationMatrix.h
      dsp/ModulationMatrix.cpp
      dsp/ExpressiveMacroMapper.h
      dsp/ExpressiveMacroMapper.cpp
      dsp/SequenceScheduler.h
      dsp/SequenceScheduler.cpp
      dsp/SequencePresets.h
      dsp/SequencePresets.cpp
      dsp/TubeRayTracer.h
      dsp/TubeRayTracer.cpp
      dsp/ElasticHallway.h
      dsp/ElasticHallway.cpp
      dsp/AlienAmplification.h
      dsp/AlienAmplification.cpp
  )
  target_include_directories(monument_smoke_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_smoke_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )
  target_link_libraries(monument_smoke_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
      # MonumentAssets # Disabled for simple JUCE UI
  )
  add_test(NAME monument_smoke_test COMMAND $<TARGET_FILE:monument_smoke_test>)

  juce_add_console_app(monument_memory_echoes_test
    PRODUCT_NAME "Monument Memory Echoes Test"
  )
  juce_generate_juce_header(monument_memory_echoes_test)
  target_sources(monument_memory_echoes_test
    PRIVATE
      tests/MemoryEchoesTest.cpp
      dsp/MemoryEchoes.cpp
      dsp/ParameterSmoother.cpp
  )
  target_include_directories(monument_memory_echoes_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_memory_echoes_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(monument_memory_echoes_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_memory_echoes_test COMMAND $<TARGET_FILE:monument_memory_echoes_test>)

  juce_add_console_app(monument_expressive_macro_demo
    PRODUCT_NAME "Monument Expressive Macro Demo"
  )
  juce_generate_juce_header(monument_expressive_macro_demo)
  target_sources(monument_expressive_macro_demo
    PRIVATE
      tests/ExpressiveMacroDemo.cpp
      dsp/ExpressiveMacroMapper.cpp
  )
  target_include_directories(monument_expressive_macro_demo
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_expressive_macro_demo
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(monument_expressive_macro_demo
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  juce_add_console_app(monument_memory_echoes_harness
    PRODUCT_NAME "Monument Memory Echoes Harness"
  )
  juce_generate_juce_header(monument_memory_echoes_harness)
  target_sources(monument_memory_echoes_harness
    PRIVATE
      tests/MemoryEchoesHarness.cpp
      dsp/MemoryEchoes.cpp
      dsp/ParameterSmoother.cpp
  )
  target_include_directories(monument_memory_echoes_harness
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_memory_echoes_harness
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(monument_memory_echoes_harness
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  juce_add_console_app(monument_experimental_modulation_test
    PRODUCT_NAME "Monument Experimental Modulation Test"
  )
  juce_generate_juce_header(monument_experimental_modulation_test)
  target_sources(monument_experimental_modulation_test
    PRIVATE
      tests/ExperimentalModulationTest.cpp
      dsp/ExperimentalModulation.cpp
  )
  target_include_directories(monument_experimental_modulation_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_experimental_modulation_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(monument_experimental_modulation_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_experimental_modulation_test COMMAND $<TARGET_FILE:monument_experimental_modulation_test>)

  juce_add_console_app(monument_constant_power_panning_test
    PRODUCT_NAME "Monument Constant Power Panning Test"
  )
  juce_generate_juce_header(monument_constant_power_panning_test)
  target_sources(monument_constant_power_panning_test
    PRIVATE
      tests/ConstantPowerPanningTest.cpp
      dsp/DspModules.cpp
  )
  target_include_directories(monument_constant_power_panning_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_compile_definitions(monument_constant_power_panning_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )
  target_link_libraries(monument_constant_power_panning_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_constant_power_panning_test COMMAND $<TARGET_FILE:monument_constant_power_panning_test>)
endif()

# Doppler Shift Test (Phase 3: Three-System Plan)
if(BUILD_TESTING)
  add_executable(monument_doppler_shift_test
    tests/DopplerShiftTest.cpp
    dsp/SpatialProcessor.cpp
  )
  target_include_directories(monument_doppler_shift_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(monument_doppler_shift_test PRIVATE
      juce::juce_audio_basics
      juce::juce_core
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_doppler_shift_test COMMAND $<TARGET_FILE:monument_doppler_shift_test>)
endif()

# SequenceScheduler Test (Phase 4: Three-System Plan)
if(BUILD_TESTING)
  add_executable(monument_sequence_scheduler_test
    tests/SequenceSchedulerTest.cpp
    dsp/SequenceScheduler.cpp
    dsp/SequencePresets.cpp
  )
  target_include_directories(monument_sequence_scheduler_test PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
  )
  target_link_libraries(monument_sequence_scheduler_test PRIVATE
      juce::juce_audio_basics
      juce::juce_core
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
  add_test(NAME monument_sequence_scheduler_test COMMAND $<TARGET_FILE:monument_sequence_scheduler_test>)
endif()

# Parameter Smoothing Test (Phase 4: Enhanced Testing)
if(BUILD_TESTING)
  juce_add_console_app(monument_parameter_smoothing_test
    PRODUCT_NAME "Monument Parameter Smoothing Test"
  )

  juce_generate_juce_header(monument_parameter_smoothing_test)

  target_sources(monument_parameter_smoothing_test
    PRIVATE
      tests/ParameterSmoothingTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_parameter_smoothing_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_parameter_smoothing_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_parameter_smoothing_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_parameter_smoothing_test COMMAND $<TARGET_FILE:monument_parameter_smoothing_test>)
endif()

# Macro Mode Integration Test
if(BUILD_TESTING)
  juce_add_console_app(monument_macro_mode_integration_test
    PRODUCT_NAME "Monument Macro Mode Integration Test"
  )

  juce_generate_juce_header(monument_macro_mode_integration_test)

  target_sources(monument_macro_mode_integration_test
    PRIVATE
      tests/MacroModeIntegrationTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_macro_mode_integration_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_macro_mode_integration_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_macro_mode_integration_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_macro_mode_integration_test COMMAND $<TARGET_FILE:monument_macro_mode_integration_test>)
endif()

# Timeline Integration Test
if(BUILD_TESTING)
  juce_add_console_app(monument_timeline_integration_test
    PRODUCT_NAME "Monument Timeline Integration Test"
  )

  juce_generate_juce_header(monument_timeline_integration_test)

  target_sources(monument_timeline_integration_test
    PRIVATE
      tests/TimelineIntegrationTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_timeline_integration_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_timeline_integration_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_timeline_integration_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_timeline_integration_test COMMAND $<TARGET_FILE:monument_timeline_integration_test>)
endif()

# Output Safety Clip Test
if(BUILD_TESTING)
  juce_add_console_app(monument_output_safety_clip_test
    PRODUCT_NAME "Monument Output Safety Clip Test"
  )

  juce_generate_juce_header(monument_output_safety_clip_test)

  target_sources(monument_output_safety_clip_test
    PRIVATE
      tests/OutputSafetyClipTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_output_safety_clip_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_output_safety_clip_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_output_safety_clip_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_output_safety_clip_test COMMAND $<TARGET_FILE:monument_output_safety_clip_test>)
endif()

# Stereo Width Test (Phase 4: Enhanced Testing)
if(BUILD_TESTING)
  juce_add_console_app(monument_stereo_width_test
    PRODUCT_NAME "Monument Stereo Width Test"
  )

  juce_generate_juce_header(monument_stereo_width_test)

  target_sources(monument_stereo_width_test
    PRIVATE
      tests/StereoWidthTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_stereo_width_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_stereo_width_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_stereo_width_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_stereo_width_test COMMAND $<TARGET_FILE:monument_stereo_width_test>)
endif()

# Latency Test (Phase 4: Enhanced Testing)
if(BUILD_TESTING)
  juce_add_console_app(monument_latency_test
    PRODUCT_NAME "Monument Latency Test"
  )

  juce_generate_juce_header(monument_latency_test)

  target_sources(monument_latency_test
    PRIVATE
      tests/LatencyTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_latency_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_latency_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_latency_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_latency_test COMMAND $<TARGET_FILE:monument_latency_test>)
endif()

# State Management Test (Phase 4: Enhanced Testing)
if(BUILD_TESTING)
  juce_add_console_app(monument_state_management_test
    PRODUCT_NAME "Monument State Management Test"
  )

  juce_generate_juce_header(monument_state_management_test)

  target_sources(monument_state_management_test
    PRIVATE
      tests/StateManagementTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_state_management_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_state_management_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_state_management_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_state_management_test COMMAND $<TARGET_FILE:monument_state_management_test>)
endif()

# DspRoutingGraph Test (Phase 1: Critical Infrastructure)
if(BUILD_TESTING)
  juce_add_console_app(monument_dsp_routing_graph_test
    PRODUCT_NAME "Monument DspRoutingGraph Test"
  )

  juce_generate_juce_header(monument_dsp_routing_graph_test)

  target_sources(monument_dsp_routing_graph_test
    PRIVATE
      tests/DspRoutingGraphTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_dsp_routing_graph_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_dsp_routing_graph_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_dsp_routing_graph_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_dsp_routing_graph_test COMMAND $<TARGET_FILE:monument_dsp_routing_graph_test>)
endif()

# Feedback Mix Safety Test (Regression: 100% mix feedback runaway)
if(BUILD_TESTING)
  juce_add_console_app(monument_feedback_mix_safety_test
    PRODUCT_NAME "Monument Feedback Mix Safety Test"
  )

  juce_generate_juce_header(monument_feedback_mix_safety_test)

  target_sources(monument_feedback_mix_safety_test
    PRIVATE
      tests/FeedbackMixSafetyTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_feedback_mix_safety_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_feedback_mix_safety_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_feedback_mix_safety_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_feedback_mix_safety_test COMMAND $<TARGET_FILE:monument_feedback_mix_safety_test>)
endif()

# ModulationMatrix Test (Phase 1.2: Critical Infrastructure)
if(BUILD_TESTING)
  juce_add_console_app(monument_modulation_matrix_test
    PRODUCT_NAME "Monument ModulationMatrix Test"
  )

  juce_generate_juce_header(monument_modulation_matrix_test)

  target_sources(monument_modulation_matrix_test
    PRIVATE
      tests/ModulationMatrixTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_modulation_matrix_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_modulation_matrix_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_modulation_matrix_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_modulation_matrix_test COMMAND $<TARGET_FILE:monument_modulation_matrix_test>)
endif()

# Novel Algorithms Test (Phase 2: Novel Algorithm Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_novel_algorithms_test
    PRODUCT_NAME "Monument Novel Algorithms Test"
  )

  juce_generate_juce_header(monument_novel_algorithms_test)

  target_sources(monument_novel_algorithms_test
    PRIVATE
      tests/NovelAlgorithmsTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_novel_algorithms_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_novel_algorithms_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_novel_algorithms_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_novel_algorithms_test COMMAND $<TARGET_FILE:monument_novel_algorithms_test>)
endif()

# Foundation Module Test (Phase 3: Foundation Module Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_foundation_test
    PRODUCT_NAME "Monument Foundation Test"
  )

  juce_generate_juce_header(monument_foundation_test)

  target_sources(monument_foundation_test
    PRIVATE
      tests/FoundationTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_foundation_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_foundation_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_foundation_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_foundation_test COMMAND $<TARGET_FILE:monument_foundation_test>)
endif()

# ParameterBuffer Test (Phase 4: Per-Sample Parameter Infrastructure)
if(BUILD_TESTING)
  juce_add_console_app(monument_parameter_buffer_test
    PRODUCT_NAME "Monument ParameterBuffer Test"
  )

  juce_generate_juce_header(monument_parameter_buffer_test)

  target_sources(monument_parameter_buffer_test
    PRIVATE
      tests/ParameterBufferTest.cpp
  )

  target_include_directories(monument_parameter_buffer_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_parameter_buffer_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_parameter_buffer_test
    PRIVATE
      juce::juce_audio_processors
      juce::juce_core
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_parameter_buffer_test COMMAND $<TARGET_FILE:monument_parameter_buffer_test>)
endif()

# Performance Benchmark Test (Stress Testing: CPU, Memory, SIMD)
if(BUILD_TESTING)
  juce_add_console_app(monument_performance_benchmark
    PRODUCT_NAME "Monument Performance Benchmark"
  )

  juce_generate_juce_header(monument_performance_benchmark)

  target_sources(monument_performance_benchmark
    PRIVATE
      tests/PerformanceBenchmarkTest.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_performance_benchmark
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_performance_benchmark
    PRIVATE
      MONUMENT_TESTING=1
      MONUMENT_BENCHMARK_MODE=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_performance_benchmark
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_performance_benchmark COMMAND $<TARGET_FILE:monument_performance_benchmark> --quick)
endif()

# Parameter Stress Test (Stress Testing: Parameter Extremes, Automation, Edge Cases)
if(BUILD_TESTING)
  juce_add_console_app(monument_parameter_stress_test
    PRODUCT_NAME "Monument Parameter Stress Test"
  )

  juce_generate_juce_header(monument_parameter_stress_test)

  target_sources(monument_parameter_stress_test
    PRIVATE
      tests/ParameterStressTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_parameter_stress_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_parameter_stress_test
    PRIVATE
      MONUMENT_TESTING=1
      MONUMENT_STRESS_TEST_MODE=1
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e7520
      JucePlugin_PluginCode=0x4d6e7220
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_parameter_stress_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_audio_processors
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_parameter_stress_test COMMAND $<TARGET_FILE:monument_parameter_stress_test> --quick)
endif()

# Pillars Zipper Test (Phase 5: Fractional Delay Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_pillars_zipper_test
    PRODUCT_NAME "Monument Pillars Zipper Test"
  )

  juce_generate_juce_header(monument_pillars_zipper_test)

  target_sources(monument_pillars_zipper_test
    PRIVATE
      tests/PillarsZipperTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_pillars_zipper_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_pillars_zipper_test
    PRIVATE
      MONUMENT_TESTING=1
      MONUMENT_ZIPPER_TEST_MODE=1
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e7520
      JucePlugin_PluginCode=0x4d6e7220
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
  )

  target_link_libraries(monument_pillars_zipper_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_audio_processors
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_pillars_zipper_test COMMAND $<TARGET_FILE:monument_pillars_zipper_test> --quick)
endif()

# DSP Initialization Test (Phase A: DSP Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_dsp_initialization_test
    PRODUCT_NAME "Monument DSP Initialization Test"
  )

  juce_generate_juce_header(monument_dsp_initialization_test)

  target_sources(monument_dsp_initialization_test
    PRIVATE
      tests/DspInitializationTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_dsp_initialization_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_dsp_initialization_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_dsp_initialization_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_dsp_initialization_test COMMAND $<TARGET_FILE:monument_dsp_initialization_test>)
endif()

# Reverb DSP Test (Phase C: DSP Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_reverb_dsp_test
    PRODUCT_NAME "Monument Reverb DSP Test"
  )

  juce_generate_juce_header(monument_reverb_dsp_test)

  target_sources(monument_reverb_dsp_test
    PRIVATE
      tests/ReverbDspTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_reverb_dsp_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_reverb_dsp_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_reverb_dsp_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_reverb_dsp_test COMMAND $<TARGET_FILE:monument_reverb_dsp_test>)
endif()

# Delay DSP Test (Phase B: DSP Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_delay_dsp_test
    PRODUCT_NAME "Monument Delay DSP Test"
  )

  juce_generate_juce_header(monument_delay_dsp_test)

  target_sources(monument_delay_dsp_test
    PRIVATE
      tests/DelayDspTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_delay_dsp_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_delay_dsp_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_delay_dsp_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_delay_dsp_test COMMAND $<TARGET_FILE:monument_delay_dsp_test>)
endif()

# Spatial DSP Test (Phase S: DSP Verification)
if(BUILD_TESTING)
  juce_add_console_app(monument_spatial_dsp_test
    PRODUCT_NAME "Monument Spatial DSP Test"
  )

  juce_generate_juce_header(monument_spatial_dsp_test)

  target_sources(monument_spatial_dsp_test
    PRIVATE
      tests/SpatialDspTest.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_spatial_dsp_test
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_spatial_dsp_test
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_spatial_dsp_test
    PRIVATE
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  add_test(NAME monument_spatial_dsp_test COMMAND $<TARGET_FILE:monument_spatial_dsp_test>)
endif()

# Plugin Analyzer Tool
option(MONUMENT_BUILD_ANALYZER "Build Monument plugin analyzer tool" ON)

if(MONUMENT_BUILD_ANALYZER)
  juce_add_console_app(monument_plugin_analyzer
    PRODUCT_NAME "Monument Plugin Analyzer"
  )

  juce_generate_juce_header(monument_plugin_analyzer)

  target_sources(monument_plugin_analyzer
    PRIVATE
      tools/plugin-analyzer/src/main.cpp
      tools/plugin-analyzer/src/PluginLoader.h
      tools/plugin-analyzer/src/PluginLoader.cpp
      tools/plugin-analyzer/src/TestSignalGenerator.h
      tools/plugin-analyzer/src/TestSignalGenerator.cpp
      tools/plugin-analyzer/src/AudioCapture.h
      tools/plugin-analyzer/src/AudioCapture.cpp
  )

  target_include_directories(monument_plugin_analyzer
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/tools/plugin-analyzer/src
  )

  target_compile_definitions(monument_plugin_analyzer
    PRIVATE
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JUCE_PLUGINHOST_VST3=1
      JUCE_PLUGINHOST_AU=1
  )

  target_link_libraries(monument_plugin_analyzer
    PRIVATE
      juce::juce_audio_utils
      juce::juce_audio_formats
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )
endif()

# ============================================================================
# QA Harness Integration (Audio DSP QA Framework)
# ============================================================================

option(BUILD_QA_HARNESS "Build QA harness test executable" OFF)

if(BUILD_QA_HARNESS)
  set(_qa_harness_submodule_dir "${CMAKE_CURRENT_SOURCE_DIR}/external/audio-dsp-qa-harness")

  if(EXISTS "${_qa_harness_submodule_dir}/CMakeLists.txt")
    # Prefer in-repo submodule for reproducible CI and local builds
    if(NOT TARGET qa_core)
      set(BUILD_QA_TESTS OFF CACHE BOOL "Disable harness tests when embedded" FORCE)
      set(BUILD_QA_EXAMPLES OFF CACHE BOOL "Disable harness examples when embedded" FORCE)
      add_subdirectory(external/audio-dsp-qa-harness EXCLUDE_FROM_ALL)
    endif()
  else()
    # Fallback for environments using a preinstalled harness package
    find_package(audio_dsp_qa_harness REQUIRED
      PATHS $ENV{HOME}/.local/lib/cmake/audio_dsp_qa_harness /usr/local/lib/cmake/audio_dsp_qa_harness
    )
  endif()

  set(_qa_core_target "")
  set(_qa_runners_target "")
  set(_qa_scenario_engine_target "")

  if(TARGET qa_core AND TARGET qa_runners AND TARGET qa_scenario_engine)
    set(_qa_core_target qa_core)
    set(_qa_runners_target qa_runners)
    set(_qa_scenario_engine_target qa_scenario_engine)
  elseif(TARGET qa::qa_core AND TARGET qa::qa_runners AND TARGET qa::qa_scenario_engine)
    set(_qa_core_target qa::qa_core)
    set(_qa_runners_target qa::qa_runners)
    set(_qa_scenario_engine_target qa::qa_scenario_engine)
  else()
    message(FATAL_ERROR "QA harness targets not found (expected qa_core/qa_runners/qa_scenario_engine or qa:: namespaced variants)")
  endif()

  # Create QA test executable
  juce_add_console_app(monument_qa
    PRODUCT_NAME "Monument QA Harness"
  )

  juce_generate_juce_header(monument_qa)

  target_sources(monument_qa
    PRIVATE
      qa/monument_adapter.h
      qa/monument_adapter.cpp
      qa/main.cpp
      plugin/PluginProcessor.cpp
      plugin/PresetManager.cpp
      ${DSP_SOURCES}
  )

  target_include_directories(monument_qa
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}
  )

  target_compile_definitions(monument_qa
    PRIVATE
      MONUMENT_TESTING=1
      JUCE_WEB_BROWSER=0
      JUCE_USE_CURL=0
      JUCE_VST3_CAN_REPLACE_VST2=0
      JucePlugin_Name="Monument"
      JucePlugin_Manufacturer="Monument Audio"
      JucePlugin_ManufacturerCode=0x4d6e6d74
      JucePlugin_PluginCode=0x4d6e6d74
  )

  target_link_libraries(monument_qa
    PRIVATE
      ${_qa_core_target}
      ${_qa_runners_target}
      ${_qa_scenario_engine_target}
      juce::juce_audio_utils
      juce::juce_dsp
    PUBLIC
      juce::juce_recommended_config_flags
      juce::juce_recommended_lto_flags
      juce::juce_recommended_warning_flags
  )

  message(STATUS "QA Harness integration enabled")
  message(STATUS "  Executable: monument_qa")
  message(STATUS "  Scenarios: scenarios/monument/")

  unset(_qa_harness_submodule_dir)
  unset(_qa_core_target)
  unset(_qa_runners_target)
  unset(_qa_scenario_engine_target)
endif()
