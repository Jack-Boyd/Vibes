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
    oscillator1.initialise ([] (float x) { return std::sin (x); }, 256);
    oscillator2.initialise ([] (float x) { return std::sin (x); }, 256);
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

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID ("osc1Gain", 1), "Osc 1 Gain",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));

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

    // Update osc2 frequency every block so octave changes take effect on held notes
    const int   octave           = (int) *apvts.getRawParameterValue ("osc2Octave");
    const float octaveMultiplier = std::pow (2.0f, (float) octave);
    oscillator2.setFrequency (currentFrequency * octaveMultiplier);

    for (const auto metadata : midiMessages)
    {
        const auto msg = metadata.getMessage();

        if (msg.isNoteOn())
        {
            currentFrequency = (float) juce::MidiMessage::getMidiNoteInHertz (msg.getNoteNumber());
            oscillator1.setFrequency (currentFrequency);
            oscillator2.setFrequency (currentFrequency * octaveMultiplier);
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

        // Oscillator 1 → channel 0
        auto block1 = juce::dsp::AudioBlock<float> (buffer).getSingleChannelBlock (0);
        oscillator1.process (juce::dsp::ProcessContextReplacing<float> (block1));

        // Oscillator 2 → monoBuffer
        monoBuffer.clear();
        auto block2 = juce::dsp::AudioBlock<float> (monoBuffer).getSubBlock (0, (size_t) numSamples);
        oscillator2.process (juce::dsp::ProcessContextReplacing<float> (block2));

        // Mix both oscillators into channel 0 with individual smoothed gains
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
