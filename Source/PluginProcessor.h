#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_graphics/juce_graphics.h>
#include <juce_dsp/juce_dsp.h>
#include <SoundTouch.h>
#include "DSP/PitchDetector.h"
#include "DSP/ScaleEngine.h"

class VoxTuneProcessor : public juce::AudioProcessor
{
public:
    VoxTuneProcessor();
    ~VoxTuneProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "VoxTune Autotune"; }
    bool   acceptsMidi()  const override { return false; }
    bool   producesMidi() const override { return false; }
    bool   isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.1; }

    int  getNumPrograms()                                  override { return 1; }
    int  getCurrentProgram()                               override { return 0; }
    void setCurrentProgram(int)                            override {}
    const juce::String getProgramName(int)                 override { return {}; }
    void changeProgramName(int, const juce::String&)       override {}

    void getStateInformation(juce::MemoryBlock& destData)  override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Data for UI
    std::atomic<float> detectedFreq { -1.0f };
    std::atomic<float> inputLevel   {  0.0f };
    std::atomic<float> outputLevel  {  0.0f };

    juce::String getDetectedNoteName() const;

private:
    // One SoundTouch instance per channel
    soundtouch::SoundTouch st[2];

    PitchDetector pitchDetector;

    // Smooth pitch ratio
    float currentPitchRatio = 1.0f;

    double currentSampleRate = 44100.0;
    int    currentBlockSize  = 512;

    // Temp buffers
    std::vector<float> monoIn, stOut;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxTuneProcessor)
};
