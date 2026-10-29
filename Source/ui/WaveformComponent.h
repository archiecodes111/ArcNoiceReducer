#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <functional>

//==============================================================================
/**
    WaveformComponent renders:
    1. Precision dual-layer audio waveform (RMS envelope + Peak contour)
    2. Interactive draggable noise-selection region with grab handles & floating badge
    3. Real-time playhead scrubber with laser guidance
    4. Top timecode calibration ruler
*/
class WaveformComponent : public juce::Component,
                          private juce::Timer
{
public:
    WaveformComponent();
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    void setAudioBuffer(const juce::AudioBuffer<float>* buffer, double sampleRate);
    void setPlayheadSample(int sampleIndex);

    int getSelectionStartSample() const noexcept { return selectionStartSample; }
    int getSelectionNumSamples() const noexcept   { return selectionNumSamples; }
    void setSelectionRange(int startSample, int numSamples);

    std::function<void(int sampleIndex)> onPlayheadScrubbed;
    std::function<void(int startSample, int numSamples)> onNoiseRegionSelected;

private:
    void timerCallback() override;
    void regenerateWaveformPath();
    int xCoordinateToSample(float x) const noexcept;
    float sampleToXCoordinate(int sample) const noexcept;

    const juce::AudioBuffer<float>* audioBuffer = nullptr;
    double sampleRate = 44100.0;
    int totalSamples = 0;

    int currentPlayheadSample = 0;

    // Selection
    int selectionStartSample = 0;
    int selectionNumSamples = 0;
    bool isDraggingSelection = false;
    bool isScrubbing = false;

    // Hover handle states
    enum class HoverTarget { None, LeftHandle, RightHandle, Body };
    HoverTarget currentHover = HoverTarget::None;

    juce::Path waveformPath;
    juce::Path envelopePath;
};
