cmake_minimum_required(VERSION 3.22)
project(MOOGER VERSION 0.5.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ANKS_BUILD_PLUGIN "Build the JUCE VST3 and Standalone" ON)
option(ANKS_BUILD_SMOKE_TEST "Build the native DSP test" ON)
if (ANKS_BUILD_SMOKE_TEST)
    enable_testing()
    add_executable(anks_audio_smoke Tests/AudioSmoke.cpp Source/SynthEngine.h)
    target_compile_features(anks_audio_smoke PRIVATE cxx_std_17)
    add_test(NAME ANKS_DSP_Smoke COMMAND anks_audio_smoke)
    add_executable(mooger_modules Tests/Modules.cpp Source/SynthEngine.h)
    target_compile_features(mooger_modules PRIVATE cxx_std_17)
    add_test(NAME MOOGER_Modules COMMAND mooger_modules)
    add_executable(mooger_percussion Tests/Percussion.cpp Source/PercussionEngine.h Source/SynthEngine.h)
    target_compile_features(mooger_percussion PRIVATE cxx_std_17)
    add_test(NAME MOOGER_Percussion COMMAND mooger_percussion)
    add_executable(mooger_modulation Tests/Modulation.cpp Source/ModulationEngine.h Source/SynthEngine.h)
    target_compile_features(mooger_modulation PRIVATE cxx_std_17)
    add_test(NAME MOOGER_Modulation COMMAND mooger_modulation)
endif()

# Permit independent DSP validation without fetching JUCE or GUI dependencies.
if (NOT ANKS_BUILD_PLUGIN)
    return()
endif()

# Local JUCE source can be supplied with -DANKS_JUCE_DIR=C:/path/to/JUCE.
# Otherwise CMake fetches a pinned JUCE 8 stable version.
if (DEFINED ANKS_JUCE_DIR AND EXISTS "${ANKS_JUCE_DIR}/CMakeLists.txt")
    add_subdirectory("${ANKS_JUCE_DIR}" "${CMAKE_BINARY_DIR}/_juce")
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.17-security
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MOOGER
    COMPANY_NAME "ANKS Labs"
    BUNDLE_ID "com.ankslabs.modularlab"
    PLUGIN_MANUFACTURER_CODE Anks
    PLUGIN_CODE Mo01
    PRODUCT_NAME "MOOGER"
    DESCRIPTION "Compact semi-modular synthesizer with independent percussion"
    FORMATS VST3 Standalone
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(MOOGER PRIVATE
    Source/SynthEngine.h
    Source/PercussionEngine.h
    Source/ModulationEngine.h
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp)

target_compile_definitions(MOOGER PUBLIC
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)

target_link_libraries(MOOGER PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_gui_basics
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

if (ANKS_BUILD_SMOKE_TEST)
    add_executable(mooger_processor_tests Tests/Processor.cpp)
    target_compile_definitions(mooger_processor_tests PRIVATE JucePlugin_Name="MOOGER" JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(mooger_processor_tests PRIVATE MOOGER juce::juce_audio_utils juce::juce_audio_processors
        juce::juce_recommended_config_flags)
    add_test(NAME MOOGER_Processor COMMAND mooger_processor_tests)
endif()

