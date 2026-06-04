#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// Parameter layout
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
VoxTuneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Retune speed: 0 = robot (instant), 100 = natural (slow)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("speed", 1), "Speed",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    // Humanize: random pitch variation like a real singer
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("humanize", 1), "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));

    // Wet/dry mix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix", 1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    // Key: 0=C, 1=C#, ..., 11=B
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("key", 1), "Key", 0, 11, 9));  // default A

    // Scale: 0=Chromatic, 1=Major, 2=Minor, 3=Pentatonic, 4=Blues, 5=Dorian
    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("scale", 1), "Scale", 0, 5, 1));  // default Major

    // Bypass
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass", 1), "Bypass", false));

    // Pitch shift (semitones, formant section)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pitchShift", 1), "Pitch Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    // Formant shift (semitones)
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("formant", 1), "Formant",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    // Formant mix
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("formantMix", 1), "Formant Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    return { params.begin(), params.end() };
}

// ─────────────────────────────────────────────────────────────────────────────
VoxTuneProcessor::VoxTuneProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "VoxTuneState", createParameterLayout())
{
}

VoxTuneProcessor::~VoxTuneProcessor() {}

// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    pitchDetector.prepare(sampleRate);
    pitchShifter.prepare(sampleRate, samplesPerBlock);
}

void VoxTuneProcessor::releaseResources()
{
    pitchDetector.reset();
    pitchShifter.reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Main audio processing
// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalChannels = getTotalNumInputChannels();
    const int numSamples    = buffer.getNumSamples();

    // ── Read parameters ───────────────────────────────────────────────────
    const bool  bypassed   = *apvts.getRawParameterValue("bypass")    > 0.5f;
    const float speed      = *apvts.getRawParameterValue("speed");
    const float humanize   = *apvts.getRawParameterValue("humanize");
    const float mix        = *apvts.getRawParameterValue("mix")       / 100.0f;
    const int   keyIdx     = (int)*apvts.getRawParameterValue("key");
    const int   scaleIdx   = (int)*apvts.getRawParameterValue("scale");
    const float pitchShift = *apvts.getRawParameterValue("pitchShift");

    // If bypassed, pass audio through unchanged
    if (bypassed)
    {
        inputLevel  = 0.0f;
        outputLevel = 0.0f;
        return;
    }

    // ── Use channel 0 for pitch detection, then process all channels ──────
    float* ch0 = buffer.getWritePointer(0);

    // Input level (for meter)
    float inRMS = 0.0f;
    for (int i = 0; i < numSamples; ++i) inRMS += ch0[i] * ch0[i];
    inputLevel = std::sqrt(inRMS / numSamples);

    // ── Pitch detection ───────────────────────────────────────────────────
    float detected = pitchDetector.process(ch0, numSamples);
    detectedFreq   = detected;

    // ── Scale snapping ────────────────────────────────────────────────────
    float target = ScaleEngine::getTargetFreq(detected, keyIdx, scaleIdx);
    targetFreq   = target;

    if (detected > 0)
    {
        float midiDet = ScaleEngine::freqToMidi(detected);
        detectedNote = (int)std::round(midiDet) % 12;
        if (detectedNote < 0) detectedNote.store(detectedNote + 12);
    }
    else
    {
        detectedNote = -1;
    }

    // ── Calculate pitch ratio ─────────────────────────────────────────────
    float pitchRatio = 1.0f;
    if (detected > 0.0f && target > 0.0f)
        pitchRatio = target / detected;

    // Additional manual pitch shift from formant section
    if (std::abs(pitchShift) > 0.001f)
        pitchRatio *= std::pow(2.0f, pitchShift / 12.0f);

    // Clamp to safe range
    pitchRatio = juce::jlimit(0.5f, 2.0f, pitchRatio);

    // ── Process each channel ──────────────────────────────────────────────
    // Create a dry copy for mix blending
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    for (int ch = 0; ch < totalChannels; ++ch)
    {
        pitchShifter.processMono(buffer.getWritePointer(ch), numSamples,
                                  pitchRatio, speed, humanize);
    }

    // ── Wet/dry blend ─────────────────────────────────────────────────────
    if (mix < 0.9999f)
    {
        for (int ch = 0; ch < totalChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = wet[i] * mix + dry[i] * (1.0f - mix);
        }
    }

    // Output level (for meter)
    float outRMS = 0.0f;
    for (int i = 0; i < numSamples; ++i) outRMS += ch0[i] * ch0[i];
    outputLevel = std::sqrt(outRMS / numSamples);
}

// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* VoxTuneProcessor::createEditor()
{
    return new VoxTuneEditor(*this);
}

juce::String VoxTuneProcessor::getDetectedNoteName() const
{
    float f = detectedFreq.load();
    return juce::String(ScaleEngine::noteName(f).c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// State save / load
// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void VoxTuneProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}

// ─────────────────────────────────────────────────────────────────────────────
// Plugin entry point
// ─────────────────────────────────────────────────────────────────────────────
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxTuneProcessor();
}
