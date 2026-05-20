/*
  ==============================================================================

    UI component for a single oscillator strip (enable, waveform, gain, octave).

  ==============================================================================
*/

#include "OscillatorComponent.h"

//==============================================================================
PowerButton::PowerButton (juce::AudioProcessorValueTreeState& a, const juce::String& paramID)
  : apvts (a), parameterID (paramID)
{
  apvts.addParameterListener (parameterID, this);
  setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

PowerButton::~PowerButton()
{
  apvts.removeParameterListener (parameterID, this);
  cancelPendingUpdate();
}

void PowerButton::parameterChanged (const juce::String&, float)
{
  triggerAsyncUpdate();
}

void PowerButton::handleAsyncUpdate()
{
  repaint();
}

void PowerButton::mouseDown (const juce::MouseEvent&)
{
  if (auto* param = apvts.getParameter (parameterID))
  {
    const bool isOn = apvts.getRawParameterValue (parameterID)->load() >= 0.5f;
    param->beginChangeGesture();
    param->setValueNotifyingHost (isOn ? 0.0f : 1.0f);
    param->endChangeGesture();
  }
}

void PowerButton::paint (juce::Graphics& g)
{
  const bool isOn = apvts.getRawParameterValue (parameterID)->load() >= 0.5f;

  const float size   = (float) juce::jmin (getWidth(), getHeight()) - 4.0f;
  const auto  circle = juce::Rectangle<float> (
    (getWidth()  - size) * 0.5f,
    (getHeight() - size) * 0.5f,
    size, size);

  if (isOn)
  {
    g.setColour (juce::Colour (0xffa6e3a1));
    g.fillEllipse (circle);
  }
  else
  {
    g.setColour (juce::Colour (0xff585b70));
    g.drawEllipse (circle.reduced (1.0f), 1.5f);
  }
}

//==============================================================================
WaveformSelector::WaveformSelector (juce::AudioProcessorValueTreeState& a,
                                    const juce::String& paramID)
  : apvts (a), parameterID (paramID)
{
  apvts.addParameterListener (parameterID, this);
  setMouseCursor (juce::MouseCursor::PointingHandCursor);
}

WaveformSelector::~WaveformSelector()
{
  apvts.removeParameterListener (parameterID, this);
  cancelPendingUpdate();
}

void WaveformSelector::parameterChanged (const juce::String&, float)
{
  triggerAsyncUpdate();
}

void WaveformSelector::handleAsyncUpdate()
{
  repaint();
}

void WaveformSelector::mouseDown (const juce::MouseEvent& e)
{
  const int clicked = juce::jlimit (0, kNumWaveforms - 1,
                                    (int) ((float) e.x / (float) getWidth() * kNumWaveforms));

  if (auto* param = apvts.getParameter (parameterID))
  {
    param->beginChangeGesture();
    param->setValueNotifyingHost (param->convertTo0to1 ((float) clicked));
    param->endChangeGesture();
  }
}

void WaveformSelector::drawWaveformIcon (juce::Graphics& g, juce::Rectangle<float> bounds,
                                         int waveIndex, bool selected)
{
  g.setColour (selected ? juce::Colour (0xff89b4fa) : juce::Colour (0xff313244));
  g.fillRoundedRectangle (bounds, 3.0f);

  const auto  r   = bounds.reduced (4.0f);
  const float x0  = r.getX(),      x1  = r.getRight();
  const float y0  = r.getY(),      y1  = r.getBottom();
  const float w   = r.getWidth(),  h   = r.getHeight();
  const float mid = r.getCentreY();

  juce::Path path;

  switch (waveIndex)
  {
    case 0: // Sine
    {
      const int steps = 32;
      for (int i = 0; i <= steps; ++i)
      {
        const float t = (float) i / steps;
        const float x = x0 + t * w;
        const float y = mid - h * 0.42f * std::sin (t * juce::MathConstants<float>::twoPi);
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
      }
      break;
    }

    case 1: // Saw — one cycle: rise left→right, instant drop at end
    {
      path.startNewSubPath (x0, y1);
      path.lineTo          (x1, y0);
      path.lineTo          (x1, y1);
      break;
    }

    case 2: // Square — 50% duty cycle
    {
      const float cx = x0 + w * 0.5f;
      path.startNewSubPath (x0, y1);
      path.lineTo          (x0, y0);
      path.lineTo          (cx, y0);
      path.lineTo          (cx, y1);
      path.lineTo          (x1, y1);
      break;
    }

    case 3: // Triangle — full cycle
    {
      path.startNewSubPath (x0,             mid);
      path.lineTo          (x0 + w * 0.25f, y0);
      path.lineTo          (x0 + w * 0.75f, y1);
      path.lineTo          (x1,             mid);
      break;
    }

    case 4: // Noise — fixed jagged pattern representing random output
    {
      static constexpr float pts[] = { 0.1f, -0.8f,  0.5f, -0.2f,  0.9f,
                                      -0.6f,  0.3f,   0.8f, -0.4f,  0.6f,
                                      -0.9f,  0.15f };
      constexpr int n = (int) (sizeof (pts) / sizeof (float));
      for (int i = 0; i < n; ++i)
      {
        const float x = x0 + ((float) i / (n - 1)) * w;
        const float y = mid + pts[i] * h * 0.45f;
        if (i == 0) path.startNewSubPath (x, y);
        else        path.lineTo (x, y);
      }
      break;
    }
  }

  g.setColour (selected ? juce::Colour (0xff1e1e2e) : juce::Colour (0xffcdd6f4));
  g.strokePath (path, juce::PathStrokeType (1.5f, juce::PathStrokeType::mitered,
                                                  juce::PathStrokeType::rounded));
}

void WaveformSelector::paint (juce::Graphics& g)
{
  const int   selected = juce::roundToInt (apvts.getRawParameterValue (parameterID)->load());
  const float iconW    = (float) getWidth() / kNumWaveforms;
  const float iconH    = (float) getHeight();

  for (int i = 0; i < kNumWaveforms; ++i)
  {
    auto bounds = juce::Rectangle<float> (i * iconW, 0.0f, iconW, iconH).reduced (1.5f);
    drawWaveformIcon (g, bounds, i, i == selected);
  }
}

//==============================================================================
OscillatorComponent::OscillatorComponent (juce::AudioProcessorValueTreeState& apvts,
                                          const juce::String& oscName,
                                          const juce::String& enabledID,
                                          const juce::String& waveformID,
                                          const juce::String& gainID,
                                          const juce::String& octaveID)
  : enableButton (apvts, enabledID),
    waveSelector (apvts, waveformID),
    gainAttach   (apvts, gainID,    gainSlider),
    octaveAttach (apvts, octaveID,  octaveSlider)
{
  nameLabel.setText (oscName, juce::dontSendNotification);
  nameLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
  nameLabel.setColour (juce::Label::textColourId, juce::Colour (0xff89b4fa));
  nameLabel.setJustificationType (juce::Justification::centred);
  addAndMakeVisible (nameLabel);

  addAndMakeVisible (enableButton);
  addAndMakeVisible (waveSelector);

  for (auto [slider, label, name] : { std::tuple { &gainSlider,   &gainLabel,   "Gain"   },
                                      std::tuple { &octaveSlider, &octaveLabel, "Octave" } })
  {
    slider->setSliderStyle (juce::Slider::RotaryVerticalDrag);
    slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 64, 18);
    addAndMakeVisible (*slider);

    label->setText (name, juce::dontSendNotification);
    label->setFont (juce::FontOptions (11.0f));
    label->setColour (juce::Label::textColourId, juce::Colour (0xff9399b2));
    label->setJustificationType (juce::Justification::centred);
    addAndMakeVisible (*label);
  }
}

void OscillatorComponent::paint (juce::Graphics& g)
{
  g.setColour (juce::Colour (0xff24273a));
  g.fillRoundedRectangle (getLocalBounds().toFloat(), 8.0f);
}

void OscillatorComponent::resized()
{
  constexpr int labelH = 18;
  constexpr int leftW  = 80;
  constexpr int waveW  = 130;

  auto row = getLocalBounds();

  // Left strip: name label top half, power button bottom half (centred, small)
  auto left = row.removeFromLeft (leftW);
  nameLabel.setBounds    (left.removeFromTop (left.getHeight() / 2).reduced (4, 0));
  enableButton.setBounds (left.withSizeKeepingCentre (24, 24));

  waveSelector.setBounds (row.removeFromLeft (waveW).reduced (4));

  const int halfW = row.getWidth() / 2;

  auto gainCol = row.removeFromLeft (halfW).reduced (8, 0);
  gainLabel.setBounds  (gainCol.removeFromBottom (labelH));
  gainSlider.setBounds (gainCol);

  auto octCol = row.reduced (8, 0);
  octaveLabel.setBounds  (octCol.removeFromBottom (labelH));
  octaveSlider.setBounds (octCol);
}
