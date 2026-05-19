/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ADSRVisualizer::ADSRVisualizer (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
    for (const auto* id : { "attack", "decay", "sustain", "release" })
        apvts.addParameterListener (id, this);
}

ADSRVisualizer::~ADSRVisualizer()
{
    for (const auto* id : { "attack", "decay", "sustain", "release" })
        apvts.removeParameterListener (id, this);
}

void ADSRVisualizer::parameterChanged (const juce::String&, float)
{
    triggerAsyncUpdate();
}

void ADSRVisualizer::handleAsyncUpdate()
{
    repaint();
}

void ADSRVisualizer::paint (juce::Graphics& g)
{
    const float attack  = *apvts.getRawParameterValue ("attack");
    const float decay   = *apvts.getRawParameterValue ("decay");
    const float sustain = *apvts.getRawParameterValue ("sustain");
    const float release = *apvts.getRawParameterValue ("release");

    auto bounds = getLocalBounds().toFloat().reduced (4.0f);

    g.setColour (juce::Colour (0xff313244));
    g.fillRoundedRectangle (bounds, 6.0f);

    const float w  = bounds.getWidth();
    const float h  = bounds.getHeight();
    const float x0 = bounds.getX();
    const float y0 = bounds.getY();

    const float holdTime = 0.4f;
    const float total    = attack + decay + holdTime + release;

    const float attackX  = x0 + (attack / total) * w;
    const float decayX   = attackX + (decay / total) * w;
    const float sustainX = decayX + (holdTime / total) * w;
    const float endX     = x0 + w;

    const float topY     = y0;
    const float sustainY = y0 + h * (1.0f - sustain);
    const float bottomY  = y0 + h;

    juce::Path path;
    path.startNewSubPath (x0,       bottomY);
    path.lineTo          (attackX,  topY);
    path.lineTo          (decayX,   sustainY);
    path.lineTo          (sustainX, sustainY);
    path.lineTo          (endX,     bottomY);

    juce::Path fill (path);
    fill.closeSubPath();
    g.setColour (juce::Colour (0x3089b4fa));
    g.fillPath (fill);

    g.setColour (juce::Colour (0xff89b4fa));
    g.strokePath (path, juce::PathStrokeType (2.0f,
                                              juce::PathStrokeType::mitered,
                                              juce::PathStrokeType::rounded));

    g.setColour (juce::Colour (0xffcdd6f4));
    constexpr float r = 3.5f;
    for (auto [px, py] : { std::pair { attackX,  topY     },
                           std::pair { decayX,   sustainY },
                           std::pair { sustainX, sustainY },
                           std::pair { endX,     bottomY  } })
        g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
}

//==============================================================================
VibesAudioProcessorEditor::VibesAudioProcessorEditor (VibesAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
      adsrVisualizer   (p.apvts),
      osc1EnableAttach (p.apvts, "osc1Enabled",  osc1EnableButton),
      osc2EnableAttach (p.apvts, "osc2Enabled",  osc2EnableButton),
      osc1WaveAttach   (p.apvts, "osc1Waveform", osc1WaveCombo),
      osc2WaveAttach   (p.apvts, "osc2Waveform", osc2WaveCombo),
      osc1GainAttach   (p.apvts, "osc1Gain",     osc1GainSlider),
      osc1OctaveAttach (p.apvts, "osc1Octave",   osc1OctaveSlider),
      osc2GainAttach   (p.apvts, "osc2Gain",     osc2GainSlider),
      osc2OctaveAttach (p.apvts, "osc2Octave",   osc2OctaveSlider),
      attackAttach     (p.apvts, "attack",       attackSlider),
      decayAttach      (p.apvts, "decay",        decaySlider),
      sustainAttach    (p.apvts, "sustain",      sustainSlider),
      releaseAttach    (p.apvts, "release",      releaseSlider)
{
    // Power button styling — green when on, dim when off
    auto configureEnableButton = [this] (juce::TextButton& btn, const juce::String& oscName)
    {
        btn.setButtonText ("ON");
        btn.setColour (juce::TextButton::buttonColourId,   juce::Colour (0xff45475a));
        btn.setColour (juce::TextButton::buttonOnColourId, juce::Colour (0xffa6e3a1));
        btn.setColour (juce::TextButton::textColourOffId,  juce::Colour (0xff6c7086));
        btn.setColour (juce::TextButton::textColourOnId,   juce::Colour (0xff1e1e2e));
        addAndMakeVisible (btn);
        juce::ignoreUnused (oscName);
    };

    // Osc section label styling
    auto configureOscLabel = [this] (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff89b4fa));
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    // Rotary slider with label below
    auto configureRotary = [this] (juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setFont (juce::FontOptions (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff9399b2));
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    // ComboBoxAttachment only syncs selection — it does NOT populate items.
    // Items must be added manually; then the initial selection is set from the
    // current parameter value (since sendInitialUpdate fired with an empty combo).
    auto configureWaveCombo = [&] (juce::ComboBox& combo, juce::Label& label,
                                   const juce::String& paramID)
    {
        combo.addItem ("Sine",     1);
        combo.addItem ("Saw",      2);
        combo.addItem ("Square",   3);
        combo.addItem ("Triangle", 4);
        combo.setSelectedId (static_cast<int> (*p.apvts.getRawParameterValue (paramID)) + 1,
                             juce::dontSendNotification);
        addAndMakeVisible (combo);

        label.setText ("Wave", juce::dontSendNotification);
        label.setFont (juce::FontOptions (11.0f));
        label.setColour (juce::Label::textColourId, juce::Colour (0xff9399b2));
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    configureOscLabel     (osc1Label,        "OSC 1");
    configureEnableButton (osc1EnableButton, "osc1");
    configureWaveCombo    (osc1WaveCombo,    osc1WaveLabel,   "osc1Waveform");
    configureRotary       (osc1GainSlider,   osc1GainLabel,   "Gain");
    configureRotary       (osc1OctaveSlider, osc1OctaveLabel, "Octave");

    configureOscLabel     (osc2Label,        "OSC 2");
    configureEnableButton (osc2EnableButton, "osc2");
    configureWaveCombo    (osc2WaveCombo,    osc2WaveLabel,   "osc2Waveform");
    configureRotary       (osc2GainSlider,   osc2GainLabel,   "Gain");
    configureRotary       (osc2OctaveSlider, osc2OctaveLabel, "Octave");

    addAndMakeVisible (adsrVisualizer);
    configureRotary (attackSlider,  attackLabel,  "Attack");
    configureRotary (decaySlider,   decayLabel,   "Decay");
    configureRotary (sustainSlider, sustainLabel, "Sustain");
    configureRotary (releaseSlider, releaseLabel, "Release");

    setSize (600, 510);
}

VibesAudioProcessorEditor::~VibesAudioProcessorEditor() {}

//==============================================================================
void VibesAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e));

    // Section backgrounds
    g.setColour (juce::Colour (0xff24273a));
    g.fillRoundedRectangle (osc1Bounds.toFloat(), 8.0f);
    g.fillRoundedRectangle (osc2Bounds.toFloat(), 8.0f);
    g.fillRoundedRectangle (adsrBounds.toFloat(), 8.0f);

    // Title
    g.setColour (juce::Colour (0xffcdd6f4));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawFittedText ("Vibes", getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void VibesAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40);

    constexpr int oscRowH  = 95;
    constexpr int gapH     = 6;
    constexpr int labelH   = 18;
    constexpr int leftW    = 100; // OSC label + enable button strip

    // Lambda: lays out one oscillator row in-place
    // Columns: [OSC label + enable btn] [wave combo] [gain rotary] [octave rotary]
    auto layoutOscRow = [&] (juce::Rectangle<int> row,
                              juce::Label&      nameLabel,
                              juce::TextButton& enableBtn,
                              juce::ComboBox&   waveCombo,  juce::Label& waveLabel,
                              juce::Slider&     gainSlider, juce::Label& gainLabel,
                              juce::Slider&     octSlider,  juce::Label& octLabel)
    {
        // Left strip — name label top half, enable button bottom half
        auto left = row.removeFromLeft (leftW);
        nameLabel.setBounds (left.removeFromTop (left.getHeight() / 2).reduced (4, 0));
        enableBtn.setBounds (left.withSizeKeepingCentre (64, 26));

        // Waveform combo — fixed 110px section, label below
        auto waveCol = row.removeFromLeft (110).reduced (8, 0);
        waveLabel.setBounds (waveCol.removeFromBottom (labelH));
        waveCombo.setBounds (waveCol.withSizeKeepingCentre (waveCol.getWidth(), 24));

        // Gain and Octave split equally across remaining width
        const int halfW = row.getWidth() / 2;

        auto gainCol = row.removeFromLeft (halfW).reduced (8, 0);
        gainLabel.setBounds (gainCol.removeFromBottom (labelH));
        gainSlider.setBounds (gainCol);

        auto octCol = row.reduced (8, 0);
        octLabel.setBounds (octCol.removeFromBottom (labelH));
        octSlider.setBounds (octCol);
    };

    // OSC 1
    osc1Bounds = area.removeFromTop (oscRowH);
    layoutOscRow (osc1Bounds,
                  osc1Label, osc1EnableButton,
                  osc1WaveCombo, osc1WaveLabel,
                  osc1GainSlider, osc1GainLabel,
                  osc1OctaveSlider, osc1OctaveLabel);
    area.removeFromTop (gapH);

    // OSC 2
    osc2Bounds = area.removeFromTop (oscRowH);
    layoutOscRow (osc2Bounds,
                  osc2Label, osc2EnableButton,
                  osc2WaveCombo, osc2WaveLabel,
                  osc2GainSlider, osc2GainLabel,
                  osc2OctaveSlider, osc2OctaveLabel);
    area.removeFromTop (gapH);

    // ADSR section — visualizer above, knobs below
    adsrBounds = area;
    auto adsrArea = area;

    adsrVisualizer.setBounds (adsrArea.removeFromTop (130));
    adsrArea.removeFromTop (4);

    const int knobW = adsrArea.getWidth() / 4;
    auto layoutAdsrKnob = [&] (juce::Slider& slider, juce::Label& label)
    {
        auto col = adsrArea.removeFromLeft (knobW);
        label.setBounds (col.removeFromBottom (labelH));
        slider.setBounds (col);
    };

    layoutAdsrKnob (attackSlider,  attackLabel);
    layoutAdsrKnob (decaySlider,   decayLabel);
    layoutAdsrKnob (sustainSlider, sustainLabel);
    layoutAdsrKnob (releaseSlider, releaseLabel);
}
