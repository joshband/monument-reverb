cmake_minimum_required(VERSION 3.22)

project(MonumentUI_Demo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch JUCE
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(JUCE)

# Add BinaryData for assets
juce_add_binary_data(MonumentUIAssets
    SOURCES
        # Panels
        Assets/panels/macro_cosmos_bg.png
        Assets/panels/foundation_bg.png
        Assets/panels/modulation_nexus_bg.png
        Assets/panels/temporal_vault_bg.png

        # Headers
        Assets/headers/macro_cosmos_header.png
        Assets/headers/foundation_header.png
        Assets/headers/modulation_nexus_header.png
        Assets/headers/temporal_vault_header.png

        # Stone knobs (base layer) - LEGACY
        Assets/knobs/stone/knob_stone_01.png
        Assets/knobs/stone/knob_stone_02.png
        Assets/knobs/stone/knob_stone_03.png
        Assets/knobs/stone/knob_stone_04.png

        # Crystal glows (RGBA overlay layer) - LEGACY
        Assets/knobs/crystal/crystal_glow_01.png
        Assets/knobs/crystal/crystal_glow_02.png
        Assets/knobs/crystal/crystal_glow_warm.png
        Assets/knobs/crystal/crystal_glow_gold.png
        Assets/knobs/crystal/crystal_glow_amber.png

        # Metal cores (center cap layer) - LEGACY
        Assets/knobs/core/core_metal_brushed_generated.png
        Assets/knobs/core/core_metal_brass_generated.png
        Assets/knobs/core/core_metal_copper_generated.png

        # Rotation indicators (pointer layer) - LEGACY
        Assets/knobs/indicator/indicator_line_generated.png
        Assets/knobs/indicator/indicator_dot_generated.png

        # === NEW PBR KNOB ASSETS (Complete layer stacks) ===

        # Geode Knob (dark crystal with blue interior)
        Assets/knobs/knob_geode/albedo.png
        Assets/knobs/knob_geode/ao.png
        Assets/knobs/knob_geode/roughness.png
        Assets/knobs/knob_geode/normal.png
        Assets/knobs/knob_geode/glow_core.png
        Assets/knobs/knob_geode/glow_crystal.png
        Assets/knobs/knob_geode/bloom.png
        Assets/knobs/knob_geode/light_wrap.png
        Assets/knobs/knob_geode/highlight.png
        Assets/knobs/knob_geode/indicator.png
        Assets/knobs/knob_geode/contact_shadow.png

        # Obsidian Knob (polished black volcanic glass)
        Assets/knobs/knob_obsidian/albedo.png
        Assets/knobs/knob_obsidian/ao.png
        Assets/knobs/knob_obsidian/roughness.png
        Assets/knobs/knob_obsidian/normal.png
        Assets/knobs/knob_obsidian/glow_core.png
        Assets/knobs/knob_obsidian/glow_crystal.png
        Assets/knobs/knob_obsidian/bloom.png
        Assets/knobs/knob_obsidian/light_wrap.png
        Assets/knobs/knob_obsidian/highlight.png
        Assets/knobs/knob_obsidian/indicator.png
        Assets/knobs/knob_obsidian/contact_shadow.png

        # Marble Knob (pale marble with veining)
        Assets/knobs/knob_marble/albedo.png
        Assets/knobs/knob_marble/ao.png
        Assets/knobs/knob_marble/roughness.png
        Assets/knobs/knob_marble/normal.png
        Assets/knobs/knob_marble/glow_core.png
        Assets/knobs/knob_marble/glow_crystal.png
        Assets/knobs/knob_marble/bloom.png
        Assets/knobs/knob_marble/light_wrap.png
        Assets/knobs/knob_marble/highlight.png
        Assets/knobs/knob_marble/indicator.png
        Assets/knobs/knob_marble/contact_shadow.png

        # Weathered Stone Knob (ancient weathered basalt)
        Assets/knobs/knob_weathered/albedo.png
        Assets/knobs/knob_weathered/ao.png
        Assets/knobs/knob_weathered/roughness.png
        Assets/knobs/knob_weathered/normal.png
        Assets/knobs/knob_weathered/glow_core.png
        Assets/knobs/knob_weathered/glow_crystal.png
        Assets/knobs/knob_weathered/bloom.png
        Assets/knobs/knob_weathered/light_wrap.png
        Assets/knobs/knob_weathered/highlight.png
        Assets/knobs/knob_weathered/indicator.png
        Assets/knobs/knob_weathered/contact_shadow.png

        # === FILMSTRIP KNOBS (Pre-composited, zero CPU cost) ===
        Assets/knobs_filmstrip/knob_geode_filmstrip.png
)

# Main GUI application
juce_add_gui_app(MonumentUI_Demo
    PRODUCT_NAME "Monument UI Demo"
    COMPANY_NAME "Monument Audio"
    BUNDLE_ID "com.monumentaudio.ui.demo"
)

target_sources(MonumentUI_Demo PRIVATE
    Source/Main.cpp
    Source/MainComponent.h
    Source/MainComponent.cpp
    Source/UI/Theme.h
    Source/UI/AssetManager.h
    Source/UI/AssetManager.cpp
    Source/UI/MonumentBodyComponent.h
    Source/UI/MonumentBodyComponent.cpp
    Source/Components/StoneKnobDemo.h
    Source/Components/StoneKnobDemo.cpp
    Source/Components/FilmstripKnobDemo.h
    Source/Components/FilmstripKnobDemo.cpp
    # WebViewKnobDemo removed - macOS WebKit security blocks local content
    # Source/Components/WebViewKnobDemo.h
    # Source/Components/WebViewKnobDemo.cpp
    # Components to be implemented:
    # Source/Components/PanelComponent.h
    # Source/Components/PanelComponent.cpp
    # Source/Components/ModulationMatrixComponent.h
    # Source/Components/ModulationMatrixComponent.cpp
)

target_compile_definitions(MonumentUI_Demo PRIVATE
    JUCE_WEB_BROWSER=1
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:MonumentUI_Demo,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:MonumentUI_Demo,JUCE_VERSION>"
)

target_link_libraries(MonumentUI_Demo PRIVATE
    MonumentUIAssets
    juce::juce_gui_basics
    juce::juce_gui_extra
    juce::juce_graphics
    juce::juce_core
PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
