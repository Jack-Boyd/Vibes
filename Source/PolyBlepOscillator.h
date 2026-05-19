/*
  ==============================================================================

    Band-limited oscillator using the polyBLEP (polynomial band-limited step)
    technique. Supports sine, sawtooth, square, and triangle waveforms.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PolyBlepOscillator
{
public:
    enum class Waveform { Sine = 0, Saw = 1, Square = 2, Triangle = 3 };

    void prepare      (double sampleRate) noexcept { sr = (float) sampleRate; }
    void setFrequency (float freq)        noexcept { phaseInc = juce::jlimit (0.0f, 0.5f, freq / sr); }
    void setWaveform  (Waveform w)        noexcept { waveform = w; }

    [[nodiscard]] float processSample() noexcept
    {
        float out = 0.0f;

        // Compute phase + 0.5 (mod 1) without std::fmod
        const float p05 = phase < 0.5f ? phase + 0.5f : phase - 0.5f;

        switch (waveform)
        {
            case Waveform::Sine:
                out = std::sin (phase * juce::MathConstants<float>::twoPi);
                break;

            case Waveform::Saw:
                out  = 2.0f * phase - 1.0f;
                out -= polyBlep (phase, phaseInc);
                break;

            case Waveform::Square:
                out  = phase < 0.5f ? 1.0f : -1.0f;
                out += polyBlep (phase, phaseInc);
                out -= polyBlep (p05, phaseInc);
                break;

            case Waveform::Triangle:
            {
                float sq  = phase < 0.5f ? 1.0f : -1.0f;
                sq += polyBlep (phase, phaseInc);
                sq -= polyBlep (p05, phaseInc);
                triState += 4.0f * phaseInc * sq;
                out = triState;
                break;
            }
        }

        phase += phaseInc;
        if (phase >= 1.0f) phase -= 1.0f;

        return out;
    }

private:
    [[nodiscard]] static float polyBlep (float t, float dt) noexcept
    {
        if (t < dt)        { t /= dt;              return t + t - t * t - 1.0f; }
        if (t > 1.0f - dt) { t = (t - 1.0f) / dt; return t * t + t + t + 1.0f; }
        return 0.0f;
    }

    Waveform waveform = Waveform::Sine;
    float    phase    = 0.0f;
    float    phaseInc = 0.0f;
    float    sr       = 44100.0f;
    float    triState = 0.0f;
};
