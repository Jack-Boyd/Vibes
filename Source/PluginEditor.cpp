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

    // Fixed visual hold duration so sustain phase is always readable
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

    // Filled area — close the path along the bottom back to origin
    juce::Path fill (path);
    fill.closeSubPath();
    g.setColour (juce::Colour (0x3089b4fa));
    g.fillPath (fill);

    // Envelope outline
    g.setColour (juce::Colour (0xff89b4fa));
    g.strokePath (path, juce::PathStrokeType (2.0f,
                                              juce::PathStrokeType::mitered,
                                              juce::PathStrokeType::rounded));

    // Dots at the four key envelope points
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
      adsrVisualizer  (p.apvts),
      attackAttach    (p.apvts, "attack",     attackSlider),
      decayAttach     (p.apvts, "decay",      decaySlider),
      sustainAttach   (p.apvts, "sustain",    sustainSlider),
      releaseAttach   (p.apvts, "release",    releaseSlider),
      osc1GainAttach  (p.apvts, "osc1Gain",   osc1GainSlider),
      osc2GainAttach  (p.apvts, "osc2Gain",   osc2GainSlider),
      osc2OctaveAttach(p.apvts, "osc2Octave", osc2OctaveSlider)
{
    addAndMakeVisible (adsrVisualizer);

    auto configureRotary = [this] (juce::Slider& slider, juce::Label& label, const juce::String& name)
    {
        slider.setSliderStyle (juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible (slider);

        label.setText (name, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        addAndMakeVisible (label);
    };

    configureRotary (attackSlider,    attackLabel,    "Attack");
    configureRotary (decaySlider,     decayLabel,     "Decay");
    configureRotary (sustainSlider,   sustainLabel,   "Sustain");
    configureRotary (releaseSlider,   releaseLabel,   "Release");
    configureRotary (osc1GainSlider,  osc1GainLabel,  "Osc 1 Gain");
    configureRotary (osc2GainSlider,  osc2GainLabel,  "Osc 2 Gain");
    configureRotary (osc2OctaveSlider, osc2OctaveLabel, "Osc 2 Octave");

    setSize (500, 490);
}

VibesAudioProcessorEditor::~VibesAudioProcessorEditor() {}

//==============================================================================
void VibesAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff1e1e2e));

    g.setColour (juce::Colour (0xffcdd6f4));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawFittedText ("Vibes", getLocalBounds().removeFromTop (40),
                      juce::Justification::centred, 1);
}

void VibesAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (10);
    area.removeFromTop (40);

    adsrVisualizer.setBounds (area.removeFromTop (160));
    area.removeFromTop (8);

    // ADSR row — 4 equally-spaced knobs
    auto adsrRow       = area.removeFromTop (145);
    const int adsrW    = adsrRow.getWidth() / 4;
    const int labelH   = 20;

    auto layoutKnob = [&] (juce::Rectangle<int>& row, int knobWidth,
                            juce::Slider& slider, juce::Label& label)
    {
        auto col = row.removeFromLeft (knobWidth);
        label.setBounds (col.removeFromBottom (labelH));
        slider.setBounds (col);
    };

    layoutKnob (adsrRow, adsrW, attackSlider,  attackLabel);
    layoutKnob (adsrRow, adsrW, decaySlider,   decayLabel);
    layoutKnob (adsrRow, adsrW, sustainSlider, sustainLabel);
    layoutKnob (adsrRow, adsrW, releaseSlider, releaseLabel);

    area.removeFromTop (8);

    // Oscillator row — 3 equally-spaced knobs
    auto oscRow     = area;
    const int oscW  = oscRow.getWidth() / 3;

    layoutKnob (oscRow, oscW, osc1GainSlider,   osc1GainLabel);
    layoutKnob (oscRow, oscW, osc2GainSlider,   osc2GainLabel);
    layoutKnob (oscRow, oscW, osc2OctaveSlider, osc2OctaveLabel);
}
