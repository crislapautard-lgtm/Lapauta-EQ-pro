cmake_minimum_required(VERSION 3.22)
project(LaPautaEQPro VERSION 1.0.0)

# ESTO ES LO IMPORTANTE: Que no pida Visual C++ en tu PC
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded")
set(CMAKE_BUILD_TYPE Release)

add_subdirectory(JUCE)

juce_add_plugin(LaPautaEQPro
    COMPANY_NAME "LaPauta"
    PLUGIN_MANUFACTURER_CODE Lapa
    PLUGIN_CODE Lpeq
    FORMATS VST3
    PRODUCT_NAME "LaPautaEQPro"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES "Fx" "EQ"
)

target_sources(LaPautaEQPro PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp
)

target_compile_definitions(LaPautaEQPro PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LaPautaEQPro PRIVATE
    juce::juce_audio_utils
    juce::juce_dsp
)

# Que lo deje como archivo solo, no carpeta
# Y liviano pa' PC de cartón
set_target_properties(LaPautaEQPro PROPERTIES
    CXX_STANDARD 17
)
