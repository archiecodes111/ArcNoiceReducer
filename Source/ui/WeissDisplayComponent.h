#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "SpectrumComponent.h"
#include "WaveformComponent.h"
#include "../dsp/DenoiseEngine.h"

//==============================================================================
/**
    WeissDisplayComponent encapsulates the iconic WEISS mastering visual display:
    - Left & Right dual-column mastering peak/RMS meters with red threshold bands
    - Central graph area housing SpectrumComponent and WaveformComponent
    - Bottom tab switcher: [ FREQUENCY ] | [ WAVEFORM ]
*/
class WeissDisplayComponent : public juce::Component,
                              private juce::Timer
{
public:
    WeissDisplayComponent(DenoiseEngine& engineRef);
    ~WeissDisplayComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    SpectrumComponent& getSpectrumComponent() noexcept { return spectrumComponent; }
    WaveformComponent& getWaveformComponent() noexcept { return waveformComponent; }

    void setOutputLevels(float leftRms, float rightRms);

private:
    void timerCallback() override;
    void drawVerticalMeter(juce::Graphics& g, const juce::Rectangle<float>& area,
                           float levelL, float levelR, const juce::String& topLabel, bool hasRedMarker);

    DenoiseEngine& engine;

    // Display mode tabs
    enum class DisplayMode { Frequency, Waveform };
    DisplayMode currentMode = DisplayMode::Frequency;

    juce::TextButton frequencyTab{"FREQUENCY"};
    juce::TextButton waveformTab{"WAVEFORM"};

    SpectrumComponent spectrumComponent;
    WaveformComponent waveformComponent;

    // Meter levels with decay
    float outLevelL = 0.0f;
    float outLevelR = 0.0f;
};
