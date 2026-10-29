cmake_minimum_required(VERSION 3.23.1)

project(NoiseCleaner VERSION 1.0.0 LANGUAGES C CXX)

# Modern C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Project metadata
set(PROJECT_NAME "NoiseCleaner")
set(PRODUCT_NAME "Arc Noise Reducer")
set(COMPANY_NAME "Archie DSP")
set(BUNDLE_ID "com.archiedsp.ArcNoiseReducer")

# Formats: Standalone & VST3 by default, plus AudioUnit (AU) on macOS
set(FORMATS Standalone VST3)
if(APPLE)
    list(APPEND FORMATS AU)
endif()

# Build options
option(COPY_PLUGIN_TO_SYSTEM "Copy plugin to system plugin folder after build" OFF)

# JUCE dependency management
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/modules/JUCE/CMakeLists.txt")
    message(STATUS "Using local JUCE from modules/JUCE")
    add_subdirectory(modules/JUCE)
else()
    message(STATUS "Local JUCE not found — fetching JUCE 8.0.12 via FetchContent...")
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.12
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# Windows runtime library configuration
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Define plugin target
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "${COMPANY_NAME}"
    BUNDLE_ID "${BUNDLE_ID}"
    COPY_PLUGIN_AFTER_BUILD ${COPY_PLUGIN_TO_SYSTEM}
    PLUGIN_MANUFACTURER_CODE ADSP
    PLUGIN_CODE NsCl
    FORMATS ${FORMATS}
    PRODUCT_NAME "${PRODUCT_NAME}"
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    IS_SYNTH FALSE
)

# Source files
target_sources(${PROJECT_NAME} PRIVATE
    Source/PluginProcessor.h
    Source/PluginProcessor.cpp
    Source/PluginEditor.h
    Source/PluginEditor.cpp

    # DSP Module
    Source/dsp/AudioFileManager.h
    Source/dsp/AudioFileManager.cpp
    Source/dsp/NoiseProfile.h
    Source/dsp/FrameProcessor.h
    Source/dsp/SpectralSubtractor.h
    Source/dsp/AudioReconstructor.h
    Source/dsp/DenoiseEngine.h
    Source/dsp/DenoiseEngine.cpp

    # UI Module
    Source/ui/CustomLookAndFeel.h
    Source/ui/CustomLookAndFeel.cpp
    Source/ui/WaveformComponent.h
    Source/ui/WaveformComponent.cpp
    Source/ui/SpectrumComponent.h
    Source/ui/SpectrumComponent.cpp
    Source/ui/WeissDisplayComponent.h
    Source/ui/WeissDisplayComponent.cpp
    Source/ui/ControlsComponent.h
    Source/ui/ControlsComponent.cpp
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/Source"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/dsp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Source/ui"
)

# Optional asset bundling
file(GLOB_RECURSE Assets CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/*.*"
)

if(Assets)
    juce_add_binary_data(AudioPluginData
        SOURCES ${Assets}
    )
    target_link_libraries(${PROJECT_NAME} PRIVATE AudioPluginData)
endif()

# Preprocessor definitions
target_compile_definitions(${PROJECT_NAME} PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0
)

# Link necessary JUCE modules
target_link_libraries(${PROJECT_NAME} PRIVATE
    juce::juce_audio_utils
    juce::juce_audio_formats
    juce::juce_dsp
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
