/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
VibesAudioProcessor::VibesAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
    // No wavetable — function is called per sample so the waveform parameter
    // can be read atomically each sample, giving seamless switching with no
    // phase discontinuity.
    oscillator1.initialise ([this] (float x) -> float {
        return *apvts.getRawParameterValue ("osc1Waveform") < 0.5f
                   ? std::sin (x)
                   : x / juce::MathConstants<float>::pi;
    });
    oscillator2.initialise ([this] (float x) -> float {
        return *apvts.getRawParameterValue ("osc2Waveform") < 0.5f
                   ? std::sin (x)
                   : x / juce::MathConstants<float>::pi;
    });
}

VibesAudioProcessor::~VibesAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VibesAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID("attack", 1),  "Attack",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID("decay", 1),   "Decay",
        juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.1f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID("sustain", 1), "Sustain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID("release", 1), "Release",
        juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.5f), 0.5f));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("osc1Enabled", 1), "Osc 1 Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("osc1Waveform", 1), "Osc 1 Waveform",
        juce::StringArray { "Sine", "Saw" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("osc1Gain", 1), "Osc 1 Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("osc1Octave", 1), "Osc 1 Octave", -2, 2, 0));

    params.push_back (std::make_unique<juce::AudioParameterBool> (
        juce::ParameterID ("osc2Enabled", 1), "Osc 2 Enabled", true));
    params.push_back (std::make_unique<juce::AudioParameterChoice> (
        juce::ParameterID ("osc2Waveform", 1), "Osc 2 Waveform",
        juce::StringArray { "Sine", "Saw" }, 0));
    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("osc2Gain", 1), "Osc 2 Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
    params.push_back (std::make_unique<juce::AudioParameterInt> (
        juce::ParameterID ("osc2Octave", 1), "Osc 2 Octave", -2, 2, 0));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String VibesAudioProcessor::getName() const { return JucePlugin_Name; }

bool VibesAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool VibesAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool VibesAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double VibesAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int  VibesAudioProcessor::getNumPrograms()                               { return 1; }
int  VibesAudioProcessor::getCurrentProgram()                            { return 0; }
void VibesAudioProcessor::setCurrentProgram (int)                        {}
const juce::String VibesAudioProcessor::getProgramName (int)             { return {}; }
void VibesAudioProcessor::changeProgramName (int, const juce::String&)   {}

//==============================================================================
void VibesAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate       = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels      = 1;

    oscillator1.prepare (spec);
    oscillator2.prepare (spec);
    oscillator1.setFrequency (440.0f);
    oscillator2.setFrequency (440.0f);

    monoBuffer.setSize (1, samplesPerBlock);

    adsr.setSampleRate (sampleRate);

    osc1GainSmooth.reset (sampleRate, 0.02);
    osc2GainSmooth.reset (sampleRate, 0.02);
    osc1GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc1Gain"));
    osc2GainSmooth.setCurrentAndTargetValue (*apvts.getRawParameterValue ("osc2Gain"));
}

void VibesAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool VibesAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

//==============================================================================
void VibesAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                        juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    adsr.setParameters ({
        *apvts.getRawParameterValue ("attack"),
        *apvts.getRawParameterValue ("decay"),
        *apvts.getRawParameterValue ("sustain"),
        *apvts.getRawParameterValue ("release")
    });

    osc1GainSmooth.setTargetValue (*apvts.getRawParameterValue ("osc1Gain"));
    osc2GainSmooth.setTargetValue (*apvts.getRawParameterValue ("osc2Gain"));

    const bool  osc1On   = *apvts.getRawParameterValue ("osc1Enabled") >= 0.5f;
    const bool  osc2On   = *apvts.getRawParameterValue ("osc2Enabled") >= 0.5f;

    // Update both oscillator frequencies every block so octave/enable changes
    // apply immediately to held notes
    const float osc1Mult = std::pow (2.0f, (float)(int) *apvts.getRawParameterValue ("osc1Octave"));
    const float osc2Mult = std::pow (2.0f, (float)(int) *apvts.getRawParameterValue ("osc2Octave"));
    oscillator1.setFrequency (currentFrequency * osc1Mult);
    oscillator2.setFrequency (currentFrequency * osc2Mult);

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            currentFrequency = (float) juce::MidiMessage::getMidiNoteInHertz (msg.getNoteNumber());
            oscillator1.setFrequency (currentFrequency * osc1Mult);
            oscillator2.setFrequency (currentFrequency * osc2Mult);
            adsr.noteOn();
        }
        else if (msg.isNoteOff())
        {
            adsr.noteOff();
        }
    }

    buffer.clear();

    if (adsr.isActive())
    {
        const int numSamples = buffer.getNumSamples();

        // Generate into channel 0 (osc1) and monoBuffer (osc2)
        // Disabled oscillators leave their buffer zeroed — the gain multiply handles silence
        if (osc1On)
        {
            auto block1 = juce::dsp::AudioBlock<float> (buffer).getSingleChannelBlock (0);
            oscillator1.process (juce::dsp::ProcessContextReplacing<float> (block1));
        }

        monoBuffer.clear();
        if (osc2On)
        {
            auto block2 = juce::dsp::AudioBlock<float> (monoBuffer).getSubBlock (0, (size_t) numSamples);
            oscillator2.process (juce::dsp::ProcessContextReplacing<float> (block2));
        }

        // Mix with individual smoothed gains
        auto*       ch0      = buffer.getWritePointer (0);
        const auto* osc2Data = monoBuffer.getReadPointer (0);

        for (int i = 0; i < numSamples; ++i)
        {
            const float g1 = osc1GainSmooth.getNextValue();
            const float g2 = osc2GainSmooth.getNextValue();
            ch0[i] = ch0[i] * g1 + osc2Data[i] * g2;
        }

        // Copy mixed mono to remaining output channels
        for (int ch = 1; ch < buffer.getNumChannels(); ++ch)
            buffer.copyFrom (ch, 0, buffer, 0, 0, numSamples);

        // Apply shared ADSR envelope
        adsr.applyEnvelopeToBuffer (buffer, 0, numSamples);
    }
}

//==============================================================================
bool VibesAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* VibesAudioProcessor::createEditor()
{
    return new VibesAudioProcessorEditor (*this);
}

//==============================================================================
void VibesAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void VibesAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));

    if (xml && xml->hasTagName (apvts.state.getType()))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VibesAudioProcessor();
}
