/*
  ==============================================================================

    Synthesiser sound and voice for the Vibes plugin.

  ==============================================================================
*/

#include "SynthVoice.h"

//==============================================================================
VibesSynthVoice::VibesSynthVoice (juce::AudioProcessorValueTreeState& a) : apvts (a) {}

void VibesSynthVoice::prepare (const juce::dsp::ProcessSpec& spec)
{
    oscillator1.prepare (spec.sampleRate);
    oscillator2.prepare (spec.sampleRate);

    adsr.setSampleRate (spec.sampleRate);

    osc1GainSmooth.reset (spec.sampleRate, 0.02);
    osc2GainSmooth.reset (spec.sampleRate, 0.02);
    osc1GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc1Gain"));
    osc2GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc2Gain"));

    isPrepared = true;
}

void VibesSynthVoice::startNote (int midiNoteNumber, float /*velocity*/,
                                  juce::SynthesiserSound*, int /*pitchWheelPosition*/)
{
    noteFrequency = (float) juce::MidiMessage::getMidiNoteInHertz (midiNoteNumber);

    const int osc1OctIdx = juce::jlimit (0, 4, (int) *apvts.getRawParameterValue ("osc1Octave") + 2);
    const int osc2OctIdx = juce::jlimit (0, 4, (int) *apvts.getRawParameterValue ("osc2Octave") + 2);
    oscillator1.setFrequency (noteFrequency * kOctaveMult[osc1OctIdx]);
    oscillator2.setFrequency (noteFrequency * kOctaveMult[osc2OctIdx]);

    osc1GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc1Gain"));
    osc2GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc2Gain"));

    adsr.setParameters ({
        *apvts.getRawParameterValue ("attack"),
        *apvts.getRawParameterValue ("decay"),
        *apvts.getRawParameterValue ("sustain"),
        *apvts.getRawParameterValue ("release")
    });
    adsr.noteOn();
}

void VibesSynthVoice::stopNote (float /*velocity*/, bool allowTailOff)
{
    if (allowTailOff)
    {
        adsr.noteOff();
    }
    else
    {
        adsr.reset();
        clearCurrentNote();
    }
}

void VibesSynthVoice::renderNextBlock (juce::AudioBuffer<float>& outputBuffer,
                                        int startSample, int numSamples)
{
    if (! isVoiceActive() || ! isPrepared)
        return;

    // Read parameters once per block
    adsr.setParameters ({
        *apvts.getRawParameterValue ("attack"),
        *apvts.getRawParameterValue ("decay"),
        *apvts.getRawParameterValue ("sustain"),
        *apvts.getRawParameterValue ("release")
    });

    osc1GainSmooth.setTargetValue (*apvts.getRawParameterValue ("osc1Gain"));
    osc2GainSmooth.setTargetValue (*apvts.getRawParameterValue ("osc2Gain"));

    const bool osc1On = *apvts.getRawParameterValue ("osc1Enabled") >= 0.5f;
    const bool osc2On = *apvts.getRawParameterValue ("osc2Enabled") >= 0.5f;

    oscillator1.setWaveform (static_cast<PolyBlepOscillator::Waveform> (
        (int) *apvts.getRawParameterValue ("osc1Waveform")));
    oscillator2.setWaveform (static_cast<PolyBlepOscillator::Waveform> (
        (int) *apvts.getRawParameterValue ("osc2Waveform")));

    // Update frequencies when octave knob changes on a held note
    const int osc1OctIdx = juce::jlimit (0, 4, (int) *apvts.getRawParameterValue ("osc1Octave") + 2);
    const int osc2OctIdx = juce::jlimit (0, 4, (int) *apvts.getRawParameterValue ("osc2Octave") + 2);
    oscillator1.setFrequency (noteFrequency * kOctaveMult[osc1OctIdx]);
    oscillator2.setFrequency (noteFrequency * kOctaveMult[osc2OctIdx]);

    // Cache write pointers to avoid per-sample getWritePointer calls
    const int numChannels = outputBuffer.getNumChannels();
    float* channelPtrs[8];
    for (int ch = 0; ch < numChannels; ++ch)
        channelPtrs[ch] = outputBuffer.getWritePointer (ch) + startSample;

    for (int i = 0; i < numSamples; ++i)
    {
        // Always advance both oscillators to keep phase continuous when toggled
        float s1 = oscillator1.processSample();
        float s2 = oscillator2.processSample();
        if (! osc1On) s1 = 0.0f;
        if (! osc2On) s2 = 0.0f;

        const float sample = (s1 * osc1GainSmooth.getNextValue()
                            + s2 * osc2GainSmooth.getNextValue())
                           * adsr.getNextSample();

        for (int ch = 0; ch < numChannels; ++ch)
            channelPtrs[ch][i] += sample;
    }

    if (! adsr.isActive())
        clearCurrentNote();
}
