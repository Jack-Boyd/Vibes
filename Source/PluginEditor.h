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
    void parameterChanged (const juce::String&, float) override;
    void handleAsyncUpdate() override;

    juce::AudioProcessorValueTreeState& apvts;
};

//==============================================================================
class VibesAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    VibesAudioProcessorEditor (VibesAudioProcessor&);
    ~VibesAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    VibesAudioProcessor& audioProcessor;

    // --- Oscillator 1 ---
    juce::Label      osc1Label;
    juce::TextButton osc1EnableButton;
    juce::ComboBox   osc1WaveCombo;
    juce::Label      osc1WaveLabel;
    juce::Slider     osc1GainSlider, osc1OctaveSlider;
    juce::Label      osc1GainLabel,  osc1OctaveLabel;

    // --- Oscillator 2 ---
    juce::Label      osc2Label;
    juce::TextButton osc2EnableButton;
    juce::ComboBox   osc2WaveCombo;
    juce::Label      osc2WaveLabel;
    juce::Slider     osc2GainSlider, osc2OctaveSlider;
    juce::Label      osc2GainLabel,  osc2OctaveLabel;

    // --- ADSR ---
    ADSRVisualizer   adsrVisualizer;
    juce::Slider     attackSlider, decaySlider, sustainSlider, releaseSlider;
    juce::Label      attackLabel,  decayLabel,  sustainLabel,  releaseLabel;

    // Attachments — declared after components so they are destroyed first
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    ButtonAttachment osc1EnableAttach,  osc2EnableAttach;
    ComboAttachment  osc1WaveAttach,    osc2WaveAttach;
    SliderAttachment osc1GainAttach,    osc1OctaveAttach;
    SliderAttachment osc2GainAttach,    osc2OctaveAttach;
    SliderAttachment attackAttach,      decayAttach, sustainAttach, releaseAttach;

    // Stored in resized(), used in paint() for section backgrounds
    juce::Rectangle<int> osc1Bounds, osc2Bounds, adsrBounds;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VibesAudioProcessorEditor)
};
