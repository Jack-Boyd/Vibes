/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "ADSRVisualiser.h"
#include "PluginProcessor.h"
#include "OscillatorComponent.h"

//==============================================================================
class VibesAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
  VibesAudioProcessorEditor (VibesAudioProcessor&);
  ~VibesAudioProcessorEditor() override;

  void paint   (juce::Graphics&) override;
  void resized () override;

private:
  VibesAudioProcessor& audioProcessor;

  OscillatorComponent osc1, osc2, osc3;

  ADSRVisualiser   adsrVisualizer;
  juce::Slider     attackSlider, decaySlider, sustainSlider, releaseSlider;
  juce::Label      attackLabel,  decayLabel,  sustainLabel,  releaseLabel;

  // Attachments declared after components so they are destroyed first
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  SliderAttachment attackAttach, decayAttach, sustainAttach, releaseAttach;

  juce::Rectangle<int> adsrBounds;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibesAudioProcessorEditor)
};
