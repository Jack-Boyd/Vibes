/*
  ==============================================================================

    UI component for a single oscillator strip (enable, waveform, gain, octave).

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
class PowerButton : public juce::Component,
                    private juce::AudioProcessorValueTreeState::Listener,
                    private juce::AsyncUpdater
{
public:
  PowerButton (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);
  ~PowerButton() override;

  void paint     (juce::Graphics& g) override;
  void mouseDown (const juce::MouseEvent&) override;

private:
  void parameterChanged (const juce::String&, float) override;
  void handleAsyncUpdate() override;

  juce::AudioProcessorValueTreeState& apvts;
  juce::String parameterID;
};

//==============================================================================
class WaveformSelector : public juce::Component,
                         private juce::AudioProcessorValueTreeState::Listener,
                         private juce::AsyncUpdater
{
public:
  WaveformSelector (juce::AudioProcessorValueTreeState& apvts, const juce::String& paramID);
  ~WaveformSelector() override;

  void paint     (juce::Graphics& g) override;
  void mouseDown (const juce::MouseEvent& e) override;

private:
  void parameterChanged (const juce::String&, float) override;
  void handleAsyncUpdate() override;

  void drawWaveformIcon (juce::Graphics& g, juce::Rectangle<float> bounds,
                         int waveIndex, bool selected);

  juce::AudioProcessorValueTreeState& apvts;
  juce::String parameterID;

  static constexpr int kNumWaveforms = 5;
};

//==============================================================================
class OscillatorComponent : public juce::Component
{
public:
  OscillatorComponent (juce::AudioProcessorValueTreeState& apvts,
                       const juce::String& oscName,
                       const juce::String& enabledID,
                       const juce::String& waveformID,
                       const juce::String& gainID,
                       const juce::String& octaveID);

  void paint   (juce::Graphics& g) override;
  void resized () override;

private:
  juce::Label      nameLabel;
  PowerButton      enableButton;
  WaveformSelector waveSelector;
  juce::Slider     gainSlider,   octaveSlider;
  juce::Label      gainLabel,    octaveLabel;

  // Attachments declared after components so they are destroyed first
  using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
  SliderAttachment gainAttach, octaveAttach;
};
