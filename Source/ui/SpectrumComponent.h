#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include <mutex>

//==============================================================================
/**
    SpectrumComponent renders:
    1. Real-time logarithmic frequency spectrum (20 Hz - 20 kHz)
    2. Precision decibel grid (+6 dB to -84 dB)
    3. Overlaid input spectrum, average noise profile, and denoised spectrum
    4. Clean vector legend badges
*/
class SpectrumComponent : public juce::Component
{
public:
    SpectrumComponent();
    ~SpectrumComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void updateSpectra(const std::vector<float>& inputMag,
                       const std::vector<float>& noiseMag,
                       const std::vector<float>& outputMag,
                       double sampleRate);

private:
    float frequencyToX(float freqHz) const noexcept;
    float magnitudeToY(float magLinear) const noexcept;
    juce::Path buildPathForSpectrum(const std::vector<float>& magnitudes) const;

    mutable std::mutex dataMutex;
    std::vector<float> inputMagnitudes;
    std::vector<float> noiseMagnitudes;
    std::vector<float> outputMagnitudes;
    double currentSampleRate = 44100.0;

    static constexpr float minFreq = 20.0f;
    static constexpr float maxFreq = 20000.0f;
    static constexpr float minDb   = -84.0f;
    static constexpr float maxDb   = 6.0f;
};
