/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================


//==============================================================================
VibesAudioProcessorEditor::VibesAudioProcessorEditor (VibesAudioProcessor& p)
  : AudioProcessorEditor (&p), audioProcessor (p),
    osc1 (p.apvts, "OSC 1", "osc1Enabled", "osc1Waveform", "osc1Gain", "osc1Octave", "osc1Fine"),
    osc2 (p.apvts, "OSC 2", "osc2Enabled", "osc2Waveform", "osc2Gain", "osc2Octave", "osc2Fine"),
    osc3 (p.apvts, "OSC 3", "osc3Enabled", "osc3Waveform", "osc3Gain", "osc3Octave", "osc3Fine"),
    adsrVisualizer (p.apvts),
    attackAttach   (p.apvts, "attack",  attackSlider),
    decayAttach    (p.apvts, "decay",   decaySlider),
    sustainAttach  (p.apvts, "sustain", sustainSlider),
    releaseAttach  (p.apvts, "release", releaseSlider)
{
  setLookAndFeel (&laf);

  addAndMakeVisible (osc1);
  addAndMakeVisible (osc2);
  addAndMakeVisible (osc3);
  addAndMakeVisible (adsrVisualizer);

  for (auto [slider, label, name] : { std::tuple { &attackSlider,  &attackLabel,  "Attack"  },
                                       std::tuple { &decaySlider,   &decayLabel,   "Decay"   },
                                       std::tuple { &sustainSlider, &sustainLabel, "Sustain" },
                                       std::tuple { &releaseSlider, &releaseLabel, "Release" } })
  {
    slider->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
    addAndMakeVisible (*slider);

    label->setText (name, juce::dontSendNotification);
    label->setFont (juce::FontOptions (10.0f));
    label->setColour (juce::Label::textColourId, juce::Colour (0xff6c7086));
    label->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*label);
  }

  setSize (600, 580);
}

VibesAudioProcessorEditor::~VibesAudioProcessorEditor()
{
  setLookAndFeel (nullptr);
}

//==============================================================================
void VibesAudioProcessorEditor::paint (juce::Graphics& g)
{
  g.fillAll (juce::Colour (0xff1e1e2e));

  // OscillatorComponents draw their own backgrounds; only ADSR needs one here
  g.setColour (juce::Colour (0xff24273a));
  g.fillRoundedRectangle (adsrBounds.toFloat(), 8.0f);

  g.setColour (juce::Colour (0xffcdd6f4));
  g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
  g.drawFittedText ("Vibes", getLocalBounds().removeFromTop (40),
                    juce::Justification::centred, 1);
}

void VibesAudioProcessorEditor::resized()
{
  auto area = getLocalBounds().reduced (10);
  area.removeFromTop (40);

  constexpr int oscRowH = 90;
  constexpr int gapH    = 6;
  constexpr int labelH  = 13;

  osc1.setBounds (area.removeFromTop (oscRowH));  area.removeFromTop (gapH);
  osc2.setBounds (area.removeFromTop (oscRowH));  area.removeFromTop (gapH);
  osc3.setBounds (area.removeFromTop (oscRowH));  area.removeFromTop (gapH);

  adsrBounds = area;
  auto adsrArea = area;

  adsrVisualizer.setBounds (adsrArea.removeFromTop (80));
  adsrArea.removeFromTop (4);

  const int knobW = adsrArea.getWidth() / 4;
  for (auto [slider, label] : { std::pair { &attackSlider,  &attackLabel  },
                                 std::pair { &decaySlider,   &decayLabel   },
                                 std::pair { &sustainSlider, &sustainLabel },
                                 std::pair { &releaseSlider, &releaseLabel } })
  {
    auto col = adsrArea.removeFromLeft (knobW);
    label->setBounds  (col.removeFromBottom (labelH));
    slider->setBounds (col);
  }
}
