cmake_minimum_required(VERSION 3.22)

project(MidiGlide VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.6
    )
    FetchContent_MakeAvailable(JUCE)
endif()

juce_add_plugin(MidiGlide
    COMPANY_NAME "topalhan"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE "Tplh"
    PLUGIN_CODE "MGld"
    FORMATS VST3 Standalone
    PRODUCT_NAME "MidiGlide"
    VST3_CATEGORIES "Instrument" "Synth"
)

target_sources(MidiGlide
    PRIVATE
        Source/MidiLogEntry.h
        Source/MidiLogQueue.h
        Source/PortamentoProcessor.h
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/MidiMonitorTab.h
        Source/MidiMonitorTab.cpp
        Source/PortamentoTab.h
        Source/PortamentoTab.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_compile_definitions(MidiGlide
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(MidiGlide
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_processors
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_warning_flags
)
