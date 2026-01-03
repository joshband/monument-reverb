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
  FORMATS AU VST3
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
    dsp/MacroMapper.h
    dsp/MacroMapper.cpp
    dsp/ModulationMatrix.h
    dsp/ModulationMatrix.cpp
    ui/MonumentKnob.h
    ui/MonumentKnob.cpp
    ui/MonumentToggle.h
    ui/MonumentToggle.cpp
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
)

target_link_libraries(Monument
  PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

option(MONUMENT_ENABLE_TESTS "Build Monument tests" ON)
option(MONUMENT_ENABLE_MEMORY "Enable Memory Echoes in plugin" OFF)

if(MONUMENT_ENABLE_MEMORY)
  target_compile_definitions(Monument
    PRIVATE
      MONUMENT_ENABLE_MEMORY=1
  )
endif()

if(MONUMENT_ENABLE_TESTS)
  enable_testing()
  add_executable(monument_smoke_test
    tests/smoke-test.cpp
  )
  add_test(NAME monument_smoke_test COMMAND monument_smoke_test)

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
  add_test(NAME monument_memory_echoes_test COMMAND monument_memory_echoes_test)

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
endif()
