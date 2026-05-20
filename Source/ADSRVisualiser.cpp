/*
  ==============================================================================

    ADSRVisualiser.cpp
    Created: 20 May 2026 9:49:28am
    Author:  Jack Boyd

  ==============================================================================
*/

#include "ADSRVisualiser.h"

ADSRVisualiser::ADSRVisualiser (juce::AudioProcessorValueTreeState& a) : apvts (a)
{
  for (const auto* id : { "attack", "decay", "sustain", "release" })
    apvts.addParameterListener (id, this);
}

ADSRVisualiser::~ADSRVisualiser()
{
  for (const auto* id : { "attack", "decay", "sustain", "release" })
    apvts.removeParameterListener (id, this);
}

void ADSRVisualiser::parameterChanged (const juce::String&, float)
{
  triggerAsyncUpdate();
}

void ADSRVisualiser::handleAsyncUpdate()
{
  repaint();
}

void ADSRVisualiser::paint (juce::Graphics& g)
{
  const float attack  = *apvts.getRawParameterValue ("attack");
  const float decay   = *apvts.getRawParameterValue ("decay");
  const float sustain = *apvts.getRawParameterValue ("sustain");
  const float release = *apvts.getRawParameterValue ("release");

  auto bounds = getLocalBounds().toFloat().reduced (4.0f);

  g.setColour (juce::Colour (0xff313244));
  g.fillRoundedRectangle (bounds, 6.0f);

  const float w  = bounds.getWidth();
  const float h  = bounds.getHeight();
  const float x0 = bounds.getX();
  const float y0 = bounds.getY();

  const float holdTime = 0.4f;
  const float total    = attack + decay + holdTime + release;

  const float attackX  = x0 + (attack / total) * w;
  const float decayX   = attackX + (decay / total) * w;
  const float sustainX = decayX + (holdTime / total) * w;
  const float endX     = x0 + w;

  const float topY     = y0;
  const float sustainY = y0 + h * (1.0f - sustain);
  const float bottomY  = y0 + h;

  juce::Path path;
  path.startNewSubPath (x0,       bottomY);
  path.lineTo          (attackX,  topY);
  path.lineTo          (decayX,   sustainY);
  path.lineTo          (sustainX, sustainY);
  path.lineTo          (endX,     bottomY);

  juce::Path fill (path);
  fill.closeSubPath();
  g.setColour (juce::Colour (0x3089b4fa));
  g.fillPath (fill);

  g.setColour (juce::Colour (0xff89b4fa));
  g.strokePath (path, juce::PathStrokeType (2.0f,
                                            juce::PathStrokeType::mitered,
                                            juce::PathStrokeType::rounded));

  g.setColour (juce::Colour (0xffcdd6f4));
  constexpr float r = 3.5f;
  for (auto [px, py] : { std::pair { attackX,  topY     },
                         std::pair { decayX,   sustainY },
                         std::pair { sustainX, sustainY },
                         std::pair { endX,     bottomY  } })
    g.fillEllipse (px - r, py - r, r * 2.0f, r * 2.0f);
}
