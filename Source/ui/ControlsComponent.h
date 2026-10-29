#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "../dsp/DenoiseEngine.h"
#include <functional>

//==============================================================================
/**
    ControlsComponent partitions hardware controls into 3 structured master modules:
    - Module 1: DSP Tuning (4 large matte dials with LED indicators & digital readouts)
    - Module 2: Noise Profiling & Audition (Hero Learn Noise button + 3-way Audition switcher)
    - Module 3: Transport & File Operations (Play/Pause, Stop, Loop + Load/Export buttons)
*/
class ControlsComponent : public juce::Component
{
public:
    ControlsComponent(DenoiseEngine& engineRef);
    ~ControlsComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void setStatusMessage(const juce::String& message, bool isError = false);
    void updateMetadataDisplay();

    std::function<void()> onLoadAudioClicked;
    std::function<void()> onLoadNoiseClicked;
    std::function<void()> onExportClicked;
    std::function<void()> onLearnNoiseClicked;
    std::function<void()> onProcessClicked;
    std::function<void(AuditionMode)> onAuditionModeChanged;
    std::function<void()> onSettingsDragEnded;
    std::function<void(float)> onAlphaChanged;
    std::function<void(int)> onFFTSizeChanged;

    juce::Slider& getAlphaSlider() { return alphaSlider; }

private:
    void updateAuditionButtons(AuditionMode mode);

    DenoiseEngine& engine;

    // Module 1: DSP Knobs (Subtraction, FFT Size, Output Trim)
    juce::Label dspHeader{"", "MASTER DSP CONTROLS"};
    juce::Slider alphaSlider;
    juce::Label alphaLabel{"", "SUBTRACTION"};
    juce::Label alphaValueLabel{"", "1.50"};

    juce::Slider fftSlider;
    juce::Label fftLabel{"", "FFT SIZE"};
    juce::Label fftValueLabel{"", "2048 pts"};

    juce::Slider outputTrimSlider;
    juce::Label outputTrimLabel{"", "OUTPUT TRIM"};
    juce::Label outputTrimValueLabel{"", "0.0 dB"};

    // Module 2: Profiling & Audition
    juce::Label profileHeader{"", "PROFILING & AUDITION"};
    juce::TextButton learnNoiseButton{"1. CAPTURE PROFILE"};
    juce::TextButton processButton{"2. PROCESS AUDIO"};

    juce::Label auditionLabel{"", "AUDITION MODE:"};
    juce::TextButton denoisedButton{"DENOISED"};
    juce::TextButton bypassButton{"BYPASS"};
    juce::TextButton deltaButton{"DELTA"};

    // Module 3: Transport & File Operations
    juce::Label transportHeader{"", "TRANSPORT & FILES"};
    juce::TextButton playButton{"PLAY"};
    juce::TextButton stopButton{"STOP"};
    juce::TextButton loopButton{"LOOP ON"};

    juce::TextButton loadAudioButton{"LOAD AUDIO"};
    juce::TextButton loadNoiseButton{"LOAD NOISE"};
    juce::TextButton exportButton{"EXPORT WAV"};

    // Footer readouts
    juce::Label statusLabel;
    juce::Label metadataLabel;
};
