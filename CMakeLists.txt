cmake_minimum_required(VERSION 3.22)

project(LaPautaEQPro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ESTA ES LA LINEA QUE ARREGLA EL ERROR EN FL - NO LA BORRES
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Release>:>")

include(FetchContent)

set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(JUCE_BUILD_EXTRAS OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.6
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(JUCE)

juce_add_plugin(LaPautaEQPro
    COMPANY_NAME "La Pauta"
    BUNDLE_ID "com.lapauta.eqpro"

    PLUGIN_MANUFACTURER_CODE LpTa
    PLUGIN_CODE LpEQ

    FORMATS VST3

    PRODUCT_NAME "La Pauta EQ Pro"
    DESCRIPTION "La Pauta Professional Parametric Equalizer"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE

    EDITOR_WANTS_KEYBOARD_FOCUS TRUE

    COPY_PLUGIN_AFTER_BUILD FALSE
)

juce_generate_juce_header(LaPautaEQPro)

target_sources(LaPautaEQPro
    PRIVATE
        Source/PluginProcessor.h
        Source/PluginProcessor.cpp
        Source/PluginEditor.h
        Source/PluginEditor.cpp
)

target_compile_definitions(LaPautaEQPro
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(LaPautaEQPro
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp

    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# ESTO LO CONVIERTE EN UN SOLO ARCHIVO COMO LaPautaSL.vst3
add_custom_command(TARGET LaPautaEQPro_VST3 POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:LaPautaEQPro_VST3>"
        "${CMAKE_BINARY_DIR}/LaPautaEQPro.vst3"
    COMMENT "Creando LaPautaEQPro.vst3 como archivo unico"
)
