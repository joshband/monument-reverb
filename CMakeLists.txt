cmake_minimum_required(VERSION 3.21)

project(Monument VERSION 0.1.0 LANGUAGES CXX)

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
    GIT_TAG 7.0.9
  )
  FetchContent_MakeAvailable(juce)
endif()

juce_add_plugin(Monument
  COMPANY_NAME "Monument Audio"
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD TRUE
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
    plugin/PluginEditor.h
    plugin/PluginEditor.cpp
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
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)

option(MONUMENT_ENABLE_TESTS "Build Monument tests" ON)

if(MONUMENT_ENABLE_TESTS)
  enable_testing()
  add_executable(monument_smoke_test
    tests/smoke-test.cpp
  )
  add_test(NAME monument_smoke_test COMMAND monument_smoke_test)
endif()
