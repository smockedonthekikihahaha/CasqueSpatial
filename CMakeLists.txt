cmake_minimum_required(VERSION 3.22)
project(CasqueSpatial VERSION 1.0.0)

option(COPY_AFTER_BUILD "Copier le VST3 dans le dossier systeme apres compilation" ON)

# JUCE est telecharge automatiquement
include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(CasqueSpatial
    COMPANY_NAME "MonStudio"
    PLUGIN_MANUFACTURER_CODE Mnst
    PLUGIN_CODE Csp3
    FORMATS VST3 Standalone
    PRODUCT_NAME "Casque Spatial 3D"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD ${COPY_AFTER_BUILD})

target_sources(CasqueSpatial PRIVATE
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(CasqueSpatial PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(CasqueSpatial PRIVATE
    juce::juce_audio_utils
    PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)
