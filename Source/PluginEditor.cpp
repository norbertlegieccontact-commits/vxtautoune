#include "PluginEditor.h"
#include <cmath>

// ─────────────────────────────────────────────────────────────────────────────
// VoxLookAndFeel
// ─────────────────────────────────────────────────────────────────────────────
VoxLookAndFeel::VoxLookAndFeel()
{
    setColour(juce::Slider::rotarySliderFillColourId,   VoxColors::cyan);
    setColour(juce::Slider::rotarySliderOutlineColourId, VoxColors::surface);
    setColour(juce::Slider::thumbColourId,               VoxColors::textPrimary);
    setColour(juce::ComboBox::backgroundColourId,        VoxColors::surface);
    setColour(juce::ComboBox::outlineColourId,           VoxColors::border);
    setColour(juce::ComboBox::textColourId,              VoxColors::textPrimary);
    setColour(juce::ComboBox::arrowColourId,             VoxColors::textSec);
    setColour(juce::PopupMenu::backgroundColourId,       VoxColors::surfaceHigh);
    setColour(juce::PopupMenu::textColourId,             VoxColors::textSec);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, VoxColors::cyan.withAlpha(0.15f));
    setColour(juce::PopupMenu::highlightedTextColourId,  VoxColors::textPrimary);
}

juce::Font VoxLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::Font(juce::FontOptions("Inter", 10.0f, juce::Font::plain));
}

juce::Font VoxLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::Font(juce::FontOptions("Inter", 11.0f, juce::Font::plain));
}

void VoxLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool,
                                   int, int, int, int, juce::ComboBox& c)
{
    auto bounds = juce::Rectangle<float>(0, 0, (float)w, (float)h);
    g.setColour(VoxColors::surface);
    g.fillRoundedRectangle(bounds, 6.0f);
    g.setColour(VoxColors::border);
    g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

    // Arrow
    auto arrowZone = bounds.removeFromRight(22.0f);
    juce::Path path;
    path.addTriangle(arrowZone.getCentreX() - 4, arrowZone.getCentreY() - 2,
                     arrowZone.getCentreX() + 4, arrowZone.getCentreY() - 2,
                     arrowZone.getCentreX(),     arrowZone.getCentreY() + 3);
    g.setColour(VoxColors::textDim);
    g.fillPath(path);
}

void VoxLookAndFeel::positionComboBoxText(juce::ComboBox& c, juce::Label& l)
{
    l.setBounds(8, 0, c.getWidth() - 28, c.getHeight());
    l.setFont(getComboBoxFont(c));
}

void VoxLookAndFeel::drawRotarySlider(juce::Graphics& g,
                                       int x, int y, int w, int h,
                                       float sliderPos,
                                       float startAngle, float endAngle,
                                       juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)w, (float)h)
                  .reduced(4.0f);
    auto cx = bounds.getCentreX();
    auto cy = bounds.getCentreY();
    auto radius = juce::jmin(bounds.getWidth(), bounds.getHeight()) * 0.5f;

    // ── Track background ─────────────────────────────────────────────────
    auto trackRadius = radius - 4.0f;
    {
        juce::Path trackBg;
        trackBg.addCentredArc(cx, cy, trackRadius, trackRadius,
                               0.0f, startAngle, endAngle, true);
        g.setColour(VoxColors::surface);
        g.strokePath(trackBg, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                    juce::PathStrokeType::rounded));
    }

    // ── Filled arc ───────────────────────────────────────────────────────
    auto fillAngle = startAngle + sliderPos * (endAngle - startAngle);
    if (std::abs(fillAngle - startAngle) > 0.01f)
    {
        juce::Path fillArc;
        fillArc.addCentredArc(cx, cy, trackRadius, trackRadius,
                               0.0f, startAngle, fillAngle, true);
        g.setColour(knobArcColour);
        g.strokePath(fillArc, juce::PathStrokeType(3.0f, juce::PathStrokeType::curved,
                                                     juce::PathStrokeType::rounded));
    }

    // ── Knob body ─────────────────────────────────────────────────────────
    auto bodyRadius = radius - 8.0f;
    {
        // Outer ring (metallic sheen)
        juce::ColourGradient grad(juce::Colour(0xff4a4a4a), cx - bodyRadius * 0.4f, cy - bodyRadius * 0.4f,
                                  juce::Colour(0xff1a1a1a), cx + bodyRadius * 0.4f, cy + bodyRadius * 0.4f, true);
        g.setGradientFill(grad);
        g.fillEllipse(cx - bodyRadius, cy - bodyRadius, bodyRadius * 2.0f, bodyRadius * 2.0f);

        // Inner circle
        auto innerR = bodyRadius * 0.72f;
        juce::ColourGradient innerGrad(juce::Colour(0xff2e2e2e), cx - innerR * 0.3f, cy - innerR * 0.3f,
                                       juce::Colour(0xff0f0f0f), cx + innerR * 0.3f, cy + innerR * 0.3f, true);
        g.setGradientFill(innerGrad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

        // Highlight
        g.setColour(juce::Colour(0x40ffffff));
        g.fillEllipse(cx - innerR * 0.6f, cy - innerR * 0.8f, innerR * 0.5f, innerR * 0.3f);
    }

    // ── Pointer dot ───────────────────────────────────────────────────────
    {
        float pointerAngle = startAngle + sliderPos * (endAngle - startAngle);
        float px = cx + (bodyRadius - 4.0f) * std::sin(pointerAngle);
        float py = cy - (bodyRadius - 4.0f) * std::cos(pointerAngle);

        g.setColour(VoxColors::textPrimary);
        g.fillEllipse(px - 2.5f, py - 2.5f, 5.0f, 5.0f);

        // Subtle glow
        g.setColour(knobArcColour.withAlpha(0.5f));
        g.fillEllipse(px - 4.0f, py - 4.0f, 8.0f, 8.0f);
    }
}

void VoxLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& b,
                                           const juce::Colour&, bool, bool)
{
    auto bounds = b.getLocalBounds().toFloat().reduced(0.5f);
    bool toggled = b.getToggleState();

    if (b.getName() == "bypass" && toggled)
    {
        g.setColour(juce::Colour(0xffff4444));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(juce::Colour(0xffff4444).brighter(0.3f));
    }
    else if (b.getName() == "formant" && toggled)
    {
        g.setColour(VoxColors::purple);
        g.fillRoundedRectangle(bounds, 5.0f);
    }
    else if (b.getName() == "lowlat" && toggled)
    {
        g.setColour(VoxColors::cyan.withAlpha(0.15f));
        g.fillRoundedRectangle(bounds, 5.0f);
        g.setColour(VoxColors::cyan);
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }
    else
    {
        g.setColour(VoxColors::border);
        g.drawRoundedRectangle(bounds, 5.0f, 1.0f);
    }
}

void VoxLookAndFeel::drawButtonText(juce::Graphics& g, juce::TextButton& b,
                                     bool, bool)
{
    bool toggled = b.getToggleState();
    bool isBypass  = b.getName() == "bypass";
    bool isFormant = b.getName() == "formant";

    if (toggled && (isBypass || isFormant))
        g.setColour(juce::Colours::black);
    else if (toggled)
        g.setColour(VoxColors::cyan);
    else
        g.setColour(VoxColors::textSec);

    g.setFont(juce::Font(juce::FontOptions("Inter", 8.0f, juce::Font::bold)));
    g.drawFittedText(b.getButtonText(), b.getLocalBounds(), juce::Justification::centred, 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// WaveformDisplay
// ─────────────────────────────────────────────────────────────────────────────
WaveformDisplay::WaveformDisplay()
{
    startTimerHz(60);
}

void WaveformDisplay::timerCallback()
{
    phase  += 0.03f;
    phase2 += 0.02f;
    repaint();
}

void WaveformDisplay::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();
    float w = bounds.getWidth();
    float h = bounds.getHeight();
    float midY = h * 0.5f;

    g.fillAll(VoxColors::background.withAlpha(0.6f));

    // Primary wave (cyan)
    {
        juce::Path wave;
        bool first = true;
        for (float x = 0; x < w; x += 1.5f)
        {
            float y = midY + std::sin(x * 0.022f + phase) * (h * 0.35f)
                            + std::sin(x * 0.11f + phase * 2.2f) * 4.0f;
            if (first) { wave.startNewSubPath(x, y); first = false; }
            else        wave.lineTo(x, y);
        }
        g.setColour(VoxColors::cyan.withAlpha(active ? 0.9f : 0.5f));
        g.strokePath(wave, juce::PathStrokeType(1.8f));
    }

    // Secondary wave (magenta, lower opacity)
    {
        juce::Path wave2;
        bool first = true;
        for (float x = 0; x < w; x += 1.5f)
        {
            float y = midY + std::sin(x * 0.016f + phase2 * 1.5f) * (h * 0.25f);
            if (first) { wave2.startNewSubPath(x, y); first = false; }
            else        wave2.lineTo(x, y);
        }
        g.setColour(VoxColors::magenta.withAlpha(0.25f));
        g.strokePath(wave2, juce::PathStrokeType(1.2f));
    }

    // Note text overlay (top-right)
    g.setColour(VoxColors::textPrimary.withAlpha(0.85f));
    g.setFont(juce::Font(juce::FontOptions("Inter", 22.0f, juce::Font::bold)));
    g.drawText(currentNote, bounds.reduced(8), juce::Justification::topRight, false);

    if (currentFreq > 0.0f)
    {
        g.setColour(VoxColors::textDim);
        g.setFont(juce::Font(juce::FontOptions("Inter", 9.0f, juce::Font::plain)));
        juce::String freqStr = juce::String(currentFreq, 1) + " Hz";
        g.drawText(freqStr, bounds.withTrimmedTop(28).reduced(8), juce::Justification::topRight, false);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// LevelMeter
// ─────────────────────────────────────────────────────────────────────────────
LevelMeter::LevelMeter(juce::Colour colour) : meterColour(colour)
{
    startTimerHz(30);
}

void LevelMeter::setLevel(float rms)
{
    targetLevel = juce::jlimit(0.0f, 1.0f, rms * 4.0f);
}

void LevelMeter::timerCallback()
{
    displayLevel += (targetLevel - displayLevel) * 0.35f;
    repaint();
}

void LevelMeter::paint(juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setColour(VoxColors::surface);
    g.fillRoundedRectangle(b, 3.0f);

    float fillH = b.getHeight() * displayLevel;
    auto fill = b.withTop(b.getBottom() - fillH);
    g.setColour(meterColour);
    g.fillRoundedRectangle(fill, 3.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// VoxTuneEditor — constructor
// ─────────────────────────────────────────────────────────────────────────────
VoxTuneEditor::VoxTuneEditor(VoxTuneProcessor& p)
    : AudioProcessorEditor(&p), proc(p)
{
    setSize(700, 480);
    setLookAndFeel(&laf);

    // ── Buttons ───────────────────────────────────────────────────────────
    bypassBtn.setName("bypass");
    bypassBtn.setClickingTogglesState(true);
    bypassAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        proc.apvts, "bypass", bypassBtn);
    addAndMakeVisible(bypassBtn);

    formantBtn.setName("formant");
    formantBtn.setClickingTogglesState(true);
    formantBtn.onClick = [this] {
        formantVisible = formantBtn.getToggleState();
        resized();
    };
    addAndMakeVisible(formantBtn);

    lowLatBtn.setName("lowlat");
    lowLatBtn.setClickingTogglesState(true);
    addAndMakeVisible(lowLatBtn);

    // ── Key combo ─────────────────────────────────────────────────────────
    static const char* keyNames[] = { "C","C#","D","D#","E","F","F#","G","G#","A","A#","B" };
    for (int i = 0; i < 12; ++i) keyCombo.addItem(keyNames[i], i + 1);
    keyCombo.setSelectedId(10);  // A
    keyAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "key", keyCombo);
    keyLabel.setText("KEY", juce::dontSendNotification);
    keyLabel.setColour(juce::Label::textColourId, VoxColors::textDim);
    keyLabel.setFont(juce::Font(juce::FontOptions("Inter", 8.0f, juce::Font::bold)));
    keyLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(keyCombo);
    addAndMakeVisible(keyLabel);

    // ── Scale combo ───────────────────────────────────────────────────────
    static const char* scaleNames[] = { "Chromatic","Major","Minor","Pentatonic","Blues","Dorian" };
    for (int i = 0; i < 6; ++i) scaleCombo.addItem(scaleNames[i], i + 1);
    scaleCombo.setSelectedId(2);  // Major
    scaleAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(
        proc.apvts, "scale", scaleCombo);
    scaleLabel.setText("SCALE", juce::dontSendNotification);
    scaleLabel.setColour(juce::Label::textColourId, VoxColors::textDim);
    scaleLabel.setFont(juce::Font(juce::FontOptions("Inter", 8.0f, juce::Font::bold)));
    scaleLabel.setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(scaleCombo);
    addAndMakeVisible(scaleLabel);

    // ── Main knobs ────────────────────────────────────────────────────────
    auto setupKnob = [&](juce::Slider& s, juce::Label& nameLabel,
                          juce::Label& valLabel, const char* name,
                          juce::Colour arcColour = VoxColors::cyan)
    {
        s.setRotaryParameters(juce::MathConstants<float>::pi * 1.25f,
                               juce::MathConstants<float>::pi * 2.75f, true);
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible(s);

        nameLabel.setText(name, juce::dontSendNotification);
        nameLabel.setColour(juce::Label::textColourId, VoxColors::textSec);
        nameLabel.setFont(juce::Font(juce::FontOptions("Inter", 9.0f, juce::Font::bold)));
        nameLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(nameLabel);

        valLabel.setColour(juce::Label::textColourId, VoxColors::textSec);
        valLabel.setFont(juce::Font(juce::FontOptions("Inter", 10.0f, juce::Font::plain)));
        valLabel.setJustificationType(juce::Justification::centred);
        addAndMakeVisible(valLabel);
    };

    setupKnob(speedKnob,    speedLabel,    speedValue,    "SPEED");
    speedAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "speed", speedKnob);
    speedKnob.onValueChange = [this] {
        speedValue.setText(juce::String((int)speedKnob.getValue()) + "%",
                            juce::dontSendNotification);
    };

    laf.knobArcColour = VoxColors::cyan;
    setupKnob(humanizeKnob, humanizeLabel, humanizeValue, "HUMANIZE");
    humanizeAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "humanize", humanizeKnob);
    humanizeKnob.onValueChange = [this] {
        humanizeValue.setText(juce::String((int)humanizeKnob.getValue()) + "%",
                               juce::dontSendNotification);
    };

    setupKnob(mixKnob, mixLabel, mixValue, "MIX", VoxColors::magenta);
    mixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "mix", mixKnob);
    mixKnob.onValueChange = [this] {
        mixValue.setText(juce::String((int)mixKnob.getValue()) + "%",
                          juce::dontSendNotification);
    };

    // ── Formant section knobs ─────────────────────────────────────────────
    setupKnob(pitchShiftKnob, pitchLabel, pitchValue, "PITCH");
    pitchAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "pitchShift", pitchShiftKnob);
    pitchShiftKnob.onValueChange = [this] {
        pitchValue.setText(juce::String(pitchShiftKnob.getValue(), 1),
                            juce::dontSendNotification);
    };

    setupKnob(formantKnob, formantLabel, formantValue, "FORMANT");
    formantAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "formant", formantKnob);
    formantKnob.onValueChange = [this] {
        formantValue.setText(juce::String(formantKnob.getValue(), 1),
                              juce::dontSendNotification);
    };

    setupKnob(fMixKnob, fMixLabel, fMixLabel, "MIX", VoxColors::magenta);
    fMixLabel.setText("MIX", juce::dontSendNotification);
    fMixAtt = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        proc.apvts, "formantMix", fMixKnob);

    // Initially hide formant section
    pitchShiftKnob.setVisible(false); pitchLabel.setVisible(false); pitchValue.setVisible(false);
    formantKnob.setVisible(false);    formantLabel.setVisible(false); formantValue.setVisible(false);
    fMixKnob.setVisible(false);       fMixLabel.setVisible(false);

    // ── Meters ────────────────────────────────────────────────────────────
    inMeterLabel.setText("IN", juce::dontSendNotification);
    inMeterLabel.setColour(juce::Label::textColourId, VoxColors::textDim);
    inMeterLabel.setFont(juce::Font(juce::FontOptions("Inter", 8.0f, juce::Font::bold)));
    inMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(inMeterLabel);
    addAndMakeVisible(inputMeter);

    outMeterLabel.setText("OUT", juce::dontSendNotification);
    outMeterLabel.setColour(juce::Label::textColourId, VoxColors::textDim);
    outMeterLabel.setFont(juce::Font(juce::FontOptions("Inter", 8.0f, juce::Font::bold)));
    outMeterLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outMeterLabel);
    addAndMakeVisible(outputMeter);

    // ── Waveform ──────────────────────────────────────────────────────────
    addAndMakeVisible(waveform);

    // Initialise displayed values
    speedValue.setText("50%",  juce::dontSendNotification);
    humanizeValue.setText("20%", juce::dontSendNotification);
    mixValue.setText("100%", juce::dontSendNotification);
    pitchValue.setText("0.0",  juce::dontSendNotification);
    formantValue.setText("0.0", juce::dontSendNotification);

    startTimerHz(30);
}

VoxTuneEditor::~VoxTuneEditor()
{
    setLookAndFeel(nullptr);
}

// ─────────────────────────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneEditor::resized()
{
    auto area = getLocalBounds();

    // ── Header bar ────────────────────────────────────────────────────────
    auto header = area.removeFromTop(40);
    bypassBtn .setBounds(header.removeFromLeft(80).reduced(5, 7));
    formantBtn.setBounds(header.removeFromLeft(80).reduced(5, 7));
    // Right side preset area (placeholder)
    header.removeFromRight(160);

    // ── Logo ──────────────────────────────────────────────────────────────
    area.removeFromTop(20);   // logo painted in paint()
    area.removeFromTop(64);

    // ── Waveform visualiser ───────────────────────────────────────────────
    area.removeFromTop(4);
    waveform.setBounds(area.removeFromTop(80).reduced(20, 0));
    area.removeFromTop(8);

    // ── Key / Scale row ───────────────────────────────────────────────────
    {
        auto row = area.removeFromTop(60).reduced(20, 0);
        auto keyBox  = row.removeFromLeft(row.getWidth() / 2 - 4);
        auto scaleBox = row.withTrimmedLeft(8);

        keyLabel .setBounds(keyBox.removeFromTop(16));
        keyCombo .setBounds(keyBox.reduced(0, 4));
        scaleLabel.setBounds(scaleBox.removeFromTop(16));
        scaleCombo.setBounds(scaleBox.reduced(0, 4));
    }
    area.removeFromTop(8);

    // ── Knobs row ─────────────────────────────────────────────────────────
    {
        auto row = area.removeFromTop(110).reduced(20, 0);

        // Speed
        {
            auto col = row.removeFromLeft(80);
            speedLabel.setBounds(col.removeFromTop(16));
            speedKnob .setBounds(col.removeFromTop(64).reduced(8));
            speedValue.setBounds(col.removeFromTop(18));
        }
        row.removeFromLeft(16);

        // Humanize
        {
            auto col = row.removeFromLeft(80);
            humanizeLabel.setBounds(col.removeFromTop(16));
            humanizeKnob .setBounds(col.removeFromTop(64).reduced(8));
            humanizeValue.setBounds(col.removeFromTop(18));
        }
        row.removeFromLeft(16);

        // Mix
        {
            auto col = row.removeFromLeft(80);
            mixLabel.setBounds(col.removeFromTop(16));
            mixKnob .setBounds(col.removeFromTop(64).reduced(8));
            mixValue.setBounds(col.removeFromTop(18));
        }

        // Meters (right side)
        auto metersArea = row.removeFromRight(50);
        auto mIn  = metersArea.removeFromLeft(22);
        auto mOut = metersArea;

        inMeterLabel .setBounds(mIn.removeFromTop(14));
        inputMeter   .setBounds(mIn.removeFromTop(64).reduced(3, 0));
        outMeterLabel.setBounds(mOut.removeFromTop(14));
        outputMeter  .setBounds(mOut.removeFromTop(64).reduced(3, 0));
    }

    // ── Formant section ───────────────────────────────────────────────────
    if (formantVisible)
    {
        area.removeFromTop(4);
        auto fSection = area.removeFromTop(100).reduced(20, 0);

        auto pitchCol = fSection.removeFromLeft(80);
        pitchLabel.setBounds(pitchCol.removeFromTop(16));
        pitchShiftKnob.setBounds(pitchCol.removeFromTop(64).reduced(8));
        pitchValue.setBounds(pitchCol.removeFromTop(18));

        fSection.removeFromLeft(20);

        auto formCol = fSection.removeFromLeft(80);
        formantLabel.setBounds(formCol.removeFromTop(16));
        formantKnob .setBounds(formCol.removeFromTop(64).reduced(8));
        formantValue.setBounds(formCol.removeFromTop(18));

        fSection.removeFromLeft(30);

        auto fMixCol = fSection.removeFromLeft(80);
        fMixLabel.setBounds(fMixCol.removeFromTop(16));
        fMixKnob .setBounds(fMixCol.removeFromTop(64).reduced(8));

        pitchShiftKnob.setVisible(true); pitchLabel.setVisible(true); pitchValue.setVisible(true);
        formantKnob.setVisible(true);    formantLabel.setVisible(true); formantValue.setVisible(true);
        fMixKnob.setVisible(true);       fMixLabel.setVisible(true);
    }
    else
    {
        pitchShiftKnob.setVisible(false); pitchLabel.setVisible(false); pitchValue.setVisible(false);
        formantKnob.setVisible(false);    formantLabel.setVisible(false); formantValue.setVisible(false);
        fMixKnob.setVisible(false);       fMixLabel.setVisible(false);
    }

    // ── Footer ────────────────────────────────────────────────────────────
    auto footer = getLocalBounds().removeFromBottom(36);
    lowLatBtn.setBounds(footer.removeFromLeft(110).reduced(8, 6));
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneEditor::paint(juce::Graphics& g)
{
    // ── Background ────────────────────────────────────────────────────────
    g.fillAll(VoxColors::background);

    // Subtle radial gradient overlay
    juce::ColourGradient grad(VoxColors::cyan.withAlpha(0.03f), getWidth() * 0.5f, getHeight() * 0.1f,
                               juce::Colours::transparentBlack, getWidth() * 0.5f, getHeight() * 0.7f, true);
    g.setGradientFill(grad);
    g.fillAll();

    // ── Header separator ─────────────────────────────────────────────────
    g.setColour(VoxColors::border);
    g.fillRect(0, 40, getWidth(), 1);

    // ── Logo ──────────────────────────────────────────────────────────────
    {
        // "VOX" in large serif-style
        g.setFont(juce::Font(juce::FontOptions("Georgia", 54.0f, juce::Font::bold)));
        g.setColour(VoxColors::textPrimary);
        juce::String vox = "VOX";
        int voxW = (int)g.getCurrentFont().getStringWidth(vox);
        int logoX = (getWidth() - voxW) / 2;
        g.drawText(vox, logoX, 44, voxW + 10, 54, juce::Justification::left);

        // "TUNE" bar in centre
        int barW = 110;
        int barH = 22;
        int barX = (getWidth() - barW) / 2;
        int barY = 60;
        g.setColour(VoxColors::background.withAlpha(0.92f));
        g.fillRect(barX, barY, barW, barH);
        g.setFont(juce::Font(juce::FontOptions("Inter", 10.0f, juce::Font::bold)));
        g.setColour(VoxColors::textSec);
        g.drawText("T U N E", barX, barY, barW, barH, juce::Justification::centred);
    }

    // ── Footer separator ─────────────────────────────────────────────────
    g.setColour(VoxColors::border);
    g.fillRect(0, getHeight() - 36, getWidth(), 1);

    // ── Footer URL ───────────────────────────────────────────────────────
    g.setColour(VoxColors::textDim);
    g.setFont(juce::Font(juce::FontOptions("Inter", 10.0f, juce::Font::plain)));
    g.drawText("vox-tune.com", getLocalBounds().removeFromBottom(36),
               juce::Justification::centredRight, false);

    // ── Formant section background ────────────────────────────────────────
    if (formantVisible)
    {
        auto fBounds = juce::Rectangle<int>(20, 310, getWidth() - 40, 105).toFloat();
        g.setColour(VoxColors::surfaceHigh.withAlpha(0.8f));
        g.fillRoundedRectangle(fBounds, 12.0f);
        g.setColour(VoxColors::purple.withAlpha(0.2f));
        g.drawRoundedRectangle(fBounds, 12.0f, 1.0f);

        g.setFont(juce::Font(juce::FontOptions("Inter", 9.0f, juce::Font::bold)));
        g.setColour(VoxColors::purple);
        g.drawText("VOXSHIFT PRO", fBounds.toNearestInt(), juce::Justification::centredTop, false);
    }

    // ── Waveform border ───────────────────────────────────────────────────
    auto wfBounds = waveform.getBounds().expanded(1).toFloat();
    g.setColour(VoxColors::border);
    g.drawRoundedRectangle(wfBounds, 8.0f, 1.0f);

    // ── Key/Scale section background ──────────────────────────────────────
    g.setColour(VoxColors::surface.withAlpha(0.5f));
    g.fillRoundedRectangle(juce::Rectangle<int>(20, 175, (getWidth() / 2) - 26, 56).toFloat(), 8.0f);
    g.fillRoundedRectangle(juce::Rectangle<int>(getWidth() / 2 + 4, 175, (getWidth() / 2) - 24, 56).toFloat(), 8.0f);
}

// ─────────────────────────────────────────────────────────────────────────────
// Timer — update meters and note display
// ─────────────────────────────────────────────────────────────────────────────
void VoxTuneEditor::timerCallback()
{
    // Update meters
    inputMeter .setLevel(proc.inputLevel .load());
    outputMeter.setLevel(proc.outputLevel.load());

    // Update waveform note display
    float freq = proc.detectedFreq.load();
    juce::String noteName = proc.getDetectedNoteName();
    waveform.setNoteText(noteName, freq > 0 ? freq : 0.0f);
    waveform.setActive(freq > 0.0f);
}
