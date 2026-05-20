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
    ), apvts (*this, nullptr, "Parameters", createParameterLayout())
#endif
{
  synthesiser.addSound (new VibesSynthSound());

  for (int i = 0; i < 16; ++i)
    synthesiser.addVoice (new VibesSynthVoice (apvts));
}

VibesAudioProcessor::~VibesAudioProcessor() {}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout VibesAudioProcessor::createParameterLayout()
{
  std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("attack",  1), "Attack",
    juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.01f));

  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("decay",   1), "Decay",
    juce::NormalisableRange<float> (0.001f, 5.0f, 0.001f, 0.5f), 0.1f));

  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("sustain", 1), "Sustain",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.8f));

  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("release", 1), "Release",
    juce::NormalisableRange<float> (0.001f, 10.0f, 0.001f, 0.5f), 0.5f));

  params.push_back (std::make_unique<juce::AudioParameterBool> (
    juce::ParameterID ("osc1Enabled",  1), "Osc 1 Enabled", true));
  params.push_back (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID ("osc1Waveform", 1), "Osc 1 Waveform",
    juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Noise" }, 0));
  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("osc1Gain",     1), "Osc 1 Gain",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.7f));
  params.push_back (std::make_unique<juce::AudioParameterInt> (
    juce::ParameterID ("osc1Octave",   1), "Osc 1 Octave", -2, 2, 0));

  params.push_back (std::make_unique<juce::AudioParameterBool> (
    juce::ParameterID ("osc2Enabled",  1), "Osc 2 Enabled", true));
  params.push_back (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID ("osc2Waveform", 1), "Osc 2 Waveform",
    juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Noise" }, 0));
  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("osc2Gain",     1), "Osc 2 Gain",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
  params.push_back (std::make_unique<juce::AudioParameterInt> (
    juce::ParameterID ("osc2Octave",   1), "Osc 2 Octave", -2, 2, 0));
  
  params.push_back (std::make_unique<juce::AudioParameterBool> (
    juce::ParameterID ("osc3Enabled",  1), "Osc 3 Enabled", true));
  params.push_back (std::make_unique<juce::AudioParameterChoice> (
    juce::ParameterID ("osc3Waveform", 1), "Osc 3 Waveform",
    juce::StringArray { "Sine", "Saw", "Square", "Triangle", "Noise" }, 0));
  params.push_back (std::make_unique<juce::AudioParameterFloat> (
    juce::ParameterID ("osc3Gain",     1), "Osc 3 Gain",
    juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));
  params.push_back (std::make_unique<juce::AudioParameterInt> (
    juce::ParameterID ("osc3Octave",   1), "Osc 3 Octave", -2, 2, 0));

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
  synthesiser.setCurrentPlaybackSampleRate (sampleRate);

  juce::dsp::ProcessSpec spec;
  spec.sampleRate       = sampleRate;
  spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
  spec.numChannels      = 1;

  for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    if (auto* voice = dynamic_cast<VibesSynthVoice*> (synthesiser.getVoice (i)))
      voice->prepare (spec);
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
  buffer.clear();
  synthesiser.renderNextBlock (buffer, midiMessages, 0, buffer.getNumSamples());
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
