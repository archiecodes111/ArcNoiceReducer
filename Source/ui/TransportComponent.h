#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/DenoiseEngine.h"
#include <functional>

//==============================================================================
/**
    TransportComponent provides:
    1. Precision vector playback controls (Play/Pause, Stop, Loop)
    2. Segmented 3-Way Audition switcher (Denoised, Bypass, Residual)
    3. High-contrast digital timecode LCD readout
*/
class TransportComponent : public juce::Component,
                           private juce::Timer
{
public:
    TransportComponent(DenoiseEngine& engineRef);
    ~TransportComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    std::function<void()> onPlayClicked;
    std::function<void()> onPauseClicked;
    std::function<void()> onStopClicked;
    std::function<void(bool)> onLoopToggled;
    std::function<void(AuditionMode)> onAuditionModeChanged;

private:
    void timerCallback() override;
    void updateAuditionButtons(AuditionMode activeMode);

    DenoiseEngine& engine;

    juce::TextButton playPauseButton{"Play"};
    juce::TextButton stopButton{"Stop"};
    juce::TextButton loopButton{"Loop Off"};

    juce::Label auditionLabel{"", "Audition Mode:"};
    juce::TextButton denoisedButton{"Denoised"};
    juce::TextButton bypassButton{"Bypass"};
    juce::TextButton residualButton{"Residual (Noise)"};

    juce::Label timeLabel{"", "00:00.0 / 00:00.0"};
};
