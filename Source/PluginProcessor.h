#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_dsp/juce_dsp.h>
#include "DSP/PitchDetector.h"
#include "DSP/PitchShifter.h"
#include "DSP/ScaleEngine.h"

// ─────────────────────────────────────────────────────────────────────────────
// VoxTuneProcessor
// Hosts the full pitch correction chain:
//   1. Detect current pitch (YIN)
//   2. Snap to target note (ScaleEngine)
//   3. Shift to target pitch (granular synthesis)
//   4. Wet/dry mix
// ─────────────────────────────────────────────────────────────────────────────
class VoxTuneProcessor : public juce::AudioProcessor
{
public:
    VoxTuneProcessor();
    ~VoxTuneProcessor() override;

    // ── AudioProcessor interface ─────────────────────────────────────────
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxTune Autotune"; }

    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int  getNumPrograms()                                    override { return 1; }
    int  getCurrentProgram()                                 override { return 0; }
    void setCurrentProgram(int)                              override {}
    const juce::String getProgramName(int)                   override { return {}; }
    void changeProgramName(int, const juce::String&)         override {}

    void getStateInformation(juce::MemoryBlock& destData)    override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    // ── Parameters ───────────────────────────────────────────────────────
    juce::AudioProcessorValueTreeState apvts;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ── Data exposed to editor ───────────────────────────────────────────
    std::atomic<float> detectedFreq  { -1.0f };
    std::atomic<float> targetFreq    { -1.0f };
    std::atomic<float> inputLevel    {  0.0f };
    std::atomic<float> outputLevel   {  0.0f };
    std::atomic<int>   detectedNote  { -1    };   // MIDI note index 0-11

    juce::String getDetectedNoteName() const;

private:
    PitchDetector pitchDetector;
    PitchShifter  pitchShifter;

    double currentSampleRate = 44100.0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxTuneProcessor)
};
