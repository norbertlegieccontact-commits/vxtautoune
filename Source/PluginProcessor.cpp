#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

// ── Parameters ────────────────────────────────────────────────────────────────
juce::AudioProcessorValueTreeState::ParameterLayout
VoxTuneProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("speed",    1), "Speed",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 50.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("humanize", 1), "Humanize",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 20.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("mix",      1), "Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("key",      1), "Key",   0, 11, 9));

    params.push_back(std::make_unique<juce::AudioParameterInt>(
        juce::ParameterID("scale",    1), "Scale", 0,  5, 1));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID("bypass",   1), "Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("pitchShift", 1), "Pitch Shift",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("formant",  1), "Formant",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID("formantMix", 1), "Formant Mix",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f), 100.0f));

    return { params.begin(), params.end() };
}

// ── Constructor ───────────────────────────────────────────────────────────────
VoxTuneProcessor::VoxTuneProcessor()
    : AudioProcessor(BusesProperties()
        .withInput ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "VoxTuneState", createParameterLayout())
{
}

VoxTuneProcessor::~VoxTuneProcessor() {}

// ── Prepare ───────────────────────────────────────────────────────────────────
void VoxTuneProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize  = samplesPerBlock;

    pitchDetector.prepare(sampleRate);

    for (int ch = 0; ch < 2; ++ch)
    {
        st[ch].setSampleRate((uint)sampleRate);
        st[ch].setChannels(1);
        st[ch].setPitchSemiTones(0.0f);
        // Key settings for natural sound
        st[ch].setSetting(SETTING_USE_AA_FILTER,       1);
        st[ch].setSetting(SETTING_AA_FILTER_LENGTH,    64);
        st[ch].setSetting(SETTING_USE_QUICKSEEK,       0);
        st[ch].setSetting(SETTING_SEQUENCE_MS,         40);
        st[ch].setSetting(SETTING_SEEKWINDOW_MS,       15);
        st[ch].setSetting(SETTING_OVERLAP_MS,          8);
        st[ch].clear();
    }

    monoIn.resize(samplesPerBlock * 4, 0.0f);
    stOut .resize(samplesPerBlock * 4, 0.0f);

    currentPitchRatio = 1.0f;
}

void VoxTuneProcessor::releaseResources()
{
    for (auto& s : st) s.clear();
    pitchDetector.reset();
}

// ── Process ───────────────────────────────────────────────────────────────────
void VoxTuneProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples  = buffer.getNumSamples();
    const int numChannels = juce::jmin(buffer.getNumChannels(), 2);

    // ── Read parameters ───────────────────────────────────────────────────
    const bool  bypassed   = *apvts.getRawParameterValue("bypass")     > 0.5f;
    const float speed      = *apvts.getRawParameterValue("speed");
    const float humanize   = *apvts.getRawParameterValue("humanize");
    const float mix        = *apvts.getRawParameterValue("mix")        / 100.0f;
    const int   keyIdx     = (int)*apvts.getRawParameterValue("key");
    const int   scaleIdx   = (int)*apvts.getRawParameterValue("scale");
    const float pitchShift = *apvts.getRawParameterValue("pitchShift");

    if (bypassed) { inputLevel = 0.0f; outputLevel = 0.0f; return; }

    // ── Input level ───────────────────────────────────────────────────────
    float inRMS = 0.0f;
    const float* ch0 = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i) inRMS += ch0[i] * ch0[i];
    inputLevel = std::sqrt(inRMS / numSamples);

    // ── Pitch detection (channel 0) ───────────────────────────────────────
    float detected = pitchDetector.process(ch0, numSamples);
    detectedFreq   = detected;

    // ── Scale snap → target semitone offset ──────────────────────────────
    float semitonesNeeded = 0.0f;

    if (detected > 0.0f)
    {
        float target = ScaleEngine::getTargetFreq(detected, keyIdx, scaleIdx);
        if (target > 0.0f)
        {
            // Convert ratio to semitones
            float rawSemitones = 12.0f * std::log2(target / detected);

            // Retune speed: smooth the semitone correction
            // speed=0 → instant (robot), speed=100 → very slow (natural ~300ms)
            float msTarget = 8.0f + speed * 2.92f;
            float tc       = msTarget * 0.001f * (float)currentSampleRate;
            float alpha    = 1.0f - std::exp(-(float)numSamples / tc);

            semitonesNeeded += alpha * (rawSemitones - semitonesNeeded);
        }
    }

    // Manual pitch shift from formant section
    semitonesNeeded += pitchShift;

    // Humanize: subtle random vibrato
    if (humanize > 0.0f)
    {
        static float hPhase = 0.0f;
        hPhase += (float)numSamples * 5.5f / (float)currentSampleRate;
        float wobble = humanize / 100.0f * 0.18f * std::sin(hPhase);
        semitonesNeeded += wobble;
    }

    // ── Set SoundTouch pitch ──────────────────────────────────────────────
    for (int ch = 0; ch < numChannels; ++ch)
        st[ch].setPitchSemiTones(semitonesNeeded);

    // ── Process each channel through SoundTouch ───────────────────────────
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* input  = buffer.getReadPointer(ch);
        float*       output = buffer.getWritePointer(ch);

        // Feed samples into SoundTouch
        st[ch].putSamples(input, (uint)numSamples);

        // Receive processed samples
        int received = (int)st[ch].receiveSamples(stOut.data(), (uint)numSamples);

        if (received > 0)
        {
            // Copy output — pad with zeros if SoundTouch returned fewer samples
            for (int i = 0; i < numSamples; ++i)
                output[i] = (i < received) ? stOut[i] : 0.0f;
        }
        // If no output yet (pipeline filling), keep dry signal
    }

    // ── Wet/dry mix ───────────────────────────────────────────────────────
    if (mix < 0.9999f)
    {
        for (int ch = 0; ch < numChannels; ++ch)
        {
            auto* wet = buffer.getWritePointer(ch);
            auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < numSamples; ++i)
                wet[i] = wet[i] * mix + dry[i] * (1.0f - mix);
        }
    }

    // ── Output level ──────────────────────────────────────────────────────
    float outRMS = 0.0f;
    const float* out0 = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i) outRMS += out0[i] * out0[i];
    outputLevel = std::sqrt(outRMS / numSamples);
}

// ── Editor ────────────────────────────────────────────────────────────────────
juce::AudioProcessorEditor* VoxTuneProcessor::createEditor()
{
    return new VoxTuneEditor(*this);
}

juce::String VoxTuneProcessor::getDetectedNoteName() const
{
    return juce::String(ScaleEngine::noteName(detectedFreq.load()).c_str());
}

// ── State ─────────────────────────────────────────────────────────────────────
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

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VoxTuneProcessor();
}
