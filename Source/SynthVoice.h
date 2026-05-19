/*
  ==============================================================================

    Synthesiser sound and voice for the Vibes plugin.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PolyBlepOscillator.h"

//==============================================================================
class VibesSynthSound : public juce::SynthesiserSound
{
public:
    bool appliesToNote    (int) override { return true; }
    bool appliesToChannel (int) override { return true; }
};

//==============================================================================
class VibesSynthVoice : public juce::SynthesiserVoice
{
public:
    explicit VibesSynthVoice (juce::AudioProcessorValueTreeState& apvts);

    bool canPlaySound (juce::SynthesiserSound* s) override
    {
        return dynamic_cast<VibesSynthSound*> (s) != nullptr;
    }

    void startNote (int midiNoteNumber, float velocity,
                    juce::SynthesiserSound*, int pitchWheelPosition) override;
    void stopNote  (float velocity, bool allowTailOff) override;

    void pitchWheelMoved (int) override {}
    void controllerMoved (int, int) override {}

    void renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                          int startSample, int numSamples) override;

    void prepare (const juce::dsp::ProcessSpec& spec);

private:
    juce::AudioProcessorValueTreeState& apvts;

    PolyBlepOscillator         oscillator1, oscillator2;
    juce::ADSR                 adsr;
    juce::SmoothedValue<float> osc1GainSmooth, osc2GainSmooth;
    float                      noteFrequency = 440.0f;
    bool                       isPrepared    = false;

    // Multipliers for osc1/2Octave parameter values -2..+2 → index 0..4
    static constexpr float kOctaveMult[5] = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };
};
