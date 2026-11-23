cmake_minimum_required(VERSION 3.22)

project(MOON_MIC VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE is not vendored: point JUCE_DIR at a local JUCE checkout (tested with 9.0.4).
set(JUCE_DIR "C:/JUCE" CACHE PATH "Path to the JUCE framework")
add_subdirectory(${JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE)

# AU can only be built on macOS.
set(MOONMIC_FORMATS VST3)
if(APPLE)
    list(APPEND MOONMIC_FORMATS AU)
endif()

# ---- Embedded model data -----------------------------------------------------
juce_add_binary_data(MoonMicData
    HEADER_NAME MoonMicData.h
    NAMESPACE MoonMicData
    SOURCES
        resources/models/moonmic_signature_v1.bin)

# ---- Sources shared by the plugin and the test runner ------------------------
set(MOONMIC_CORE_SOURCES
    source/model/ModelLibrary.cpp
    source/dsp/SimpleFFT.cpp
    source/dsp/SignatureDesigner.cpp
    source/dsp/ElectronicsStage.cpp)

set(MOONMIC_COMMON_DEFS
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

# ---- Plugin --------------------------------------------------------------------
juce_add_plugin(MoonMic
    PRODUCT_NAME "MOON MIC"
    COMPANY_NAME "MOON AUDIO ENGINEERING"
    BUNDLE_ID com.moonaudioengineering.moonmic
    PLUGIN_MANUFACTURER_CODE MnAE
    PLUGIN_CODE MnMc
    FORMATS ${MOONMIC_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES Fx)

target_sources(MoonMic PRIVATE
    ${MOONMIC_CORE_SOURCES}
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/ui/ResponsePlot.cpp
    source/ui/MoonAudioLogo.cpp
    source/ui/StageView.cpp
    source/ui/PolarPlot.cpp)

target_include_directories(MoonMic PRIVATE source)
target_compile_definitions(MoonMic PUBLIC ${MOONMIC_COMMON_DEFS})

target_link_libraries(MoonMic
    PRIVATE
        MoonMicData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---- Test runner (console) -----------------------------------------------------
juce_add_console_app(MoonMicTests PRODUCT_NAME "MoonMicTests")

target_sources(MoonMicTests PRIVATE
    ${MOONMIC_CORE_SOURCES}
    tests/SignatureTests.cpp)

target_include_directories(MoonMicTests PRIVATE source)
target_compile_definitions(MoonMicTests PRIVATE
    ${MOONMIC_COMMON_DEFS}
    MOONMIC_REFERENCE_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/signature_reference_v1.json"
    MOONMIC_TRANSFORM_JSON="${CMAKE_CURRENT_SOURCE_DIR}/tests/reference/transform_reference_v1.json")

target_link_libraries(MoonMicTests
    PRIVATE
        MoonMicData
        juce::juce_core
        juce::juce_audio_basics
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)

# ---- Processor-level test (audio path, no host) ----------------------------------
juce_add_console_app(MoonMicProcessorTests PRODUCT_NAME "MoonMicProcessorTests")

target_sources(MoonMicProcessorTests PRIVATE
    ${MOONMIC_CORE_SOURCES}
    source/PluginProcessor.cpp
    source/PluginEditor.cpp
    source/ui/ResponsePlot.cpp
    source/ui/MoonAudioLogo.cpp
    source/ui/StageView.cpp
    source/ui/PolarPlot.cpp
    tests/ProcessorTests.cpp)

target_include_directories(MoonMicProcessorTests PRIVATE source)
target_compile_definitions(MoonMicProcessorTests PRIVATE ${MOONMIC_COMMON_DEFS} JUCE_MODAL_LOOPS_PERMITTED=1)

target_link_libraries(MoonMicProcessorTests
    PRIVATE
        MoonMicData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags)
