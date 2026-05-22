/*
  ==============================================================================

    Custom look and feel for the Vibes plugin.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class VibesLookAndFeel : public juce::LookAndFeel_V4
{
public:
  void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                         float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                         juce::Slider&) override
  {
    const float radius  = (float) juce::jmin (width, height) * 0.5f - 4.0f;
    const float centreX = (float) x + (float) width  * 0.5f;
    const float centreY = (float) y + (float) height * 0.5f;
    const float angle   = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float trackR  = radius * 0.86f;
    constexpr  float trackW = 3.0f;

    // Background track (full range)
    {
      juce::Path p;
      p.addCentredArc (centreX, centreY, trackR, trackR, 0.0f,
                       rotaryStartAngle, rotaryEndAngle, true);
      g.setColour (juce::Colour (0xff45475a));
      g.strokePath (p, juce::PathStrokeType (trackW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // Value arc
    {
      juce::Path p;
      p.addCentredArc (centreX, centreY, trackR, trackR, 0.0f,
                       rotaryStartAngle, angle, true);
      g.setColour (juce::Colour (0xff89b4fa));
      g.strokePath (p, juce::PathStrokeType (trackW, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));
    }

    // Knob body
    const float knobR = radius * 0.64f;
    g.setColour (juce::Colour (0xff313244));
    g.fillEllipse (centreX - knobR, centreY - knobR, knobR * 2.0f, knobR * 2.0f);

    // Indicator line
    const float cosA = std::cos (angle - juce::MathConstants<float>::halfPi);
    const float sinA = std::sin (angle - juce::MathConstants<float>::halfPi);
    g.setColour (juce::Colour (0xffcdd6f4));
    g.drawLine (centreX + cosA * knobR * 0.3f,
                centreY + sinA * knobR * 0.3f,
                centreX + cosA * knobR * 0.78f,
                centreY + sinA * knobR * 0.78f,
                1.5f);

    // Dot at track end (current value)
    g.setColour (juce::Colour (0xff89b4fa));
    const float dotR = 2.5f;
    g.fillEllipse (centreX + std::cos (angle - juce::MathConstants<float>::halfPi) * trackR - dotR,
                   centreY + std::sin (angle - juce::MathConstants<float>::halfPi) * trackR - dotR,
                   dotR * 2.0f, dotR * 2.0f);
  }
};
