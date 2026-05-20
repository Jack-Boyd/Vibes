/*
  ==============================================================================

    ADSRVisualiser.h
    Created: 20 May 2026 9:49:28am
    Author:  Jack Boyd

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class ADSRVisualiser : public juce::Component,
                       private juce::AudioProcessorValueTreeState::Listener,
                       private juce::AsyncUpdater
{
public:
  explicit ADSRVisualiser (juce::AudioProcessorValueTreeState& apvts);
  ~ADSRVisualiser() override;

  void paint (juce::Graphics& g) override;

private:
  void parameterChanged (const juce::String&, float) override;
  void handleAsyncUpdate() override;

  juce::AudioProcessorValueTreeState& apvts;
};
