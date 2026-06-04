#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// ─────────────────────────────────────────────────────────────────────────────
// VoxTune colour palette (matching the HTML mockup exactly)
// ─────────────────────────────────────────────────────────────────────────────
namespace VoxColors
{
    static const juce::Colour background  { 0xff0a0a0e };
    static const juce::Colour surface     { 0xff111115 };
    static const juce::Colour surfaceHigh { 0xff1a1a1e };
    static const juce::Colour border      { 0x18ffffff };
    static const juce::Colour textPrimary { 0xfff5f5f0 };
    static const juce::Colour textSec     { 0xff888888 };
    static const juce::Colour textDim     { 0xff555555 };
    static const juce::Colour cyan        { 0xff00e5ff };
    static const juce::Colour magenta     { 0xffff2d92 };
    static const juce::Colour purple      { 0xffa855f7 };
    static const juce::Colour cyanGlow    { 0x8000e5ff };
    static const juce::Colour magGlow     { 0x80ff2d92 };
    static const juce::Colour purpleGlow  { 0x80a855f7 };
}

// ─────────────────────────────────────────────────────────────────────────────
// Custom LookAndFeel for knobs and buttons
// ─────────────────────────────────────────────────────────────────────────────
class VoxLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VoxLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int w, int h,
                          float sliderPos, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& s) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& b,
                               const juce::Colour& bgColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText(juce::Graphics& g, juce::TextButton& b,
                         bool highlighted, bool down) override;

    juce::Font getLabelFont(juce::Label&) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawComboBox(juce::Graphics& g, int w, int h, bool isDown,
                       int, int, int, int, juce::ComboBox& c) override;
    void positionComboBoxText(juce::ComboBox& c, juce::Label& l) override;

    juce::Colour knobArcColour { VoxColors::cyan };
};

// ─────────────────────────────────────────────────────────────────────────────
// Waveform display component
// ─────────────────────────────────────────────────────────────────────────────
class WaveformDisplay : public juce::Component, private juce::Timer
{
public:
    WaveformDisplay();
    void paint(juce::Graphics& g) override;

    void setNoteText(const juce::String& note, float freq)
    {
        currentNote = note;
        currentFreq = freq;
    }

    void setActive(bool a) { active = a; }

private:
    void timerCallback() override;

    float   phase      = 0.0f;
    float   phase2     = 0.0f;
    bool    active     = false;
    juce::String currentNote { "---" };
    float        currentFreq  { 0.0f  };
};

// ─────────────────────────────────────────────────────────────────────────────
// Level meter component
// ─────────────────────────────────────────────────────────────────────────────
class LevelMeter : public juce::Component, private juce::Timer
{
public:
    explicit LevelMeter(juce::Colour colour);
    void setLevel(float rms);
    void paint(juce::Graphics& g) override;

private:
    void timerCallback() override;
    float   displayLevel = 0.0f;
    float   targetLevel  = 0.0f;
    juce::Colour meterColour;
};

// ─────────────────────────────────────────────────────────────────────────────
// Main editor
// ─────────────────────────────────────────────────────────────────────────────
class VoxTuneEditor : public juce::AudioProcessorEditor,
                      private juce::Timer
{
public:
    explicit VoxTuneEditor(VoxTuneProcessor&);
    ~VoxTuneEditor() override;

    void paint(juce::Graphics& g)   override;
    void resized()                  override;

private:
    void timerCallback() override;
    void layoutFormantSection(bool visible);

    VoxTuneProcessor& proc;
    VoxLookAndFeel    laf;

    // ── Main knobs ────────────────────────────────────────────────────────
    juce::Slider speedKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider humanizeKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider mixKnob      { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label  speedLabel, humanizeLabel, mixLabel;
    juce::Label  speedValue, humanizeValue, mixValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> speedAtt, humanizeAtt, mixAtt;

    // ── Formant section ───────────────────────────────────────────────────
    juce::Slider pitchShiftKnob { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider formantKnob    { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Slider fMixKnob       { juce::Slider::RotaryVerticalDrag, juce::Slider::NoTextBox };
    juce::Label  pitchLabel, formantLabel, fMixLabel;
    juce::Label  pitchValue, formantValue;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> pitchAtt, formantAtt, fMixAtt;

    bool formantVisible = false;

    // ── Key / Scale combos ────────────────────────────────────────────────
    juce::ComboBox keyCombo, scaleCombo;
    juce::Label    keyLabel, scaleLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> keyAtt, scaleAtt;

    // ── Buttons ───────────────────────────────────────────────────────────
    juce::TextButton bypassBtn    { "BYPASS"      };
    juce::TextButton formantBtn   { "FORMANT"     };
    juce::TextButton lowLatBtn    { "LOW LATENCY" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAtt;

    // ── Meters ────────────────────────────────────────────────────────────
    LevelMeter inputMeter  { VoxColors::cyan    };
    LevelMeter outputMeter { VoxColors::magenta };
    juce::Label inMeterLabel, outMeterLabel;

    // ── Waveform ──────────────────────────────────────────────────────────
    WaveformDisplay waveform;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(VoxTuneEditor)
};
