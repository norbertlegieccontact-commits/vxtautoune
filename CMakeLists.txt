cmake_minimum_required(VERSION 3.22)
project(VoxTuneAutotune VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Auto-download JUCE 8 (no manual install needed) ──────────────────────────
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

# ── Plugin target ─────────────────────────────────────────────────────────────
juce_add_plugin(VoxTuneAutotune
    COMPANY_NAME                "VoxTune"
    COMPANY_WEBSITE             "https://vox-tune.com"
    PLUGIN_MANUFACTURER_CODE    Vxtn
    PLUGIN_CODE                 Vtat
    FORMATS                     VST3 Standalone
    PRODUCT_NAME                "VoxTune Autotune"
    DESCRIPTION                 "Premium real-time pitch correction by VoxTune"
    IS_SYNTH                    FALSE
    NEEDS_MIDI_INPUT            FALSE
    NEEDS_MIDI_OUTPUT           FALSE
    IS_MIDI_EFFECT              FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    VST3_CATEGORIES             "Fx" "Pitch Shift"
    COPY_PLUGIN_AFTER_BUILD     FALSE
)

# ── Source files ──────────────────────────────────────────────────────────────
target_sources(VoxTuneAutotune
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/DSP/PitchDetector.cpp
        Source/DSP/PitchShifter.cpp
        Source/DSP/ScaleEngine.cpp
)

target_include_directories(VoxTuneAutotune PRIVATE Source)

# ── Compile definitions ───────────────────────────────────────────────────────
target_compile_definitions(VoxTuneAutotune
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

# ── Link libraries ────────────────────────────────────────────────────────────
target_link_libraries(VoxTuneAutotune
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_graphics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
