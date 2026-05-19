/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
class ADSRVisualizer : public juce::Component,
                       private juce::AudioProcessorValueTreeState::Listener,
                       private juce::AsyncUpdater
{
public:
    explicit ADSRVisualizer (juce::AudioProcessorValueTreeState& apvts);
    ~ADSRVisualizer() override;

    void paint (juce::Graphics& g) override;

private:
    // Called on any thread — must not touch the UI directly
    void parameterChanged (const juce::String&, float) override;

    // Called on the message thread — safe to repaint
    void handleAsyncUpdate() override;

    juce::AudioProcessorValueTreeState& apvts;
};

//==============================================================================
class VibesAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    VibesAudioProcessorEditor (VibesAudioProcessor&);
    ~VibesAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VibesAudioProcessor& audioProcessor;

    ADSRVisualizer adsrVisualizer;

    juce::Slider attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label  attackLabel,  decayLabel,  sustainLabel,  releaseLabel;

    juce::Slider osc1GainSlider, osc2GainSlider, osc2OctaveSlider;
    juce::Label  osc1GainLabel,  osc2GainLabel,  osc2OctaveLabel;

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    Attachment attackAttach,   decayAttach,    sustainAttach,    releaseAttach;
    Attachment osc1GainAttach, osc2GainAttach, osc2OctaveAttach;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibesAudioProcessorEditor)
};
