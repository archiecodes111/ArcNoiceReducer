#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include "PluginProcessor.h"
#include "ui/CustomLookAndFeel.h"
#include "ui/WeissDisplayComponent.h"
#include "ui/ControlsComponent.h"

//==============================================================================
/**
    AudioPluginAudioProcessorEditor for ARC NOISE REDUCER:
    - ARC DSP branding & geometric arc vector monogram
    - Recessed precision master display with dual vertical meters
    - 3-card control deck with high-contrast, guaranteed-legible buttons
*/
class AudioPluginAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                              public juce::FileDragAndDropTarget,
                                              private juce::Timer
{
public:
    explicit AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor&);
    ~AudioPluginAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag(const juce::StringArray& files) override;
    void filesDropped(const juce::StringArray& files, int x, int y) override;

    void loadFile(const juce::File& file);

private:
    void timerCallback() override;

    void openAudioFileChooser();
    void openNoiseFileChooser();
    void openExportFileChooser();
    void drawArcLogo(juce::Graphics& g, float x, float y, float size);

    AudioPluginAudioProcessor& processorRef;
    CustomLookAndFeel customLookAndFeel;

    WeissDisplayComponent displayComponent;
    ControlsComponent controlsComponent;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> alphaAttachment;

    std::unique_ptr<juce::FileChooser> fileChooser;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessorEditor)
};

using NoiseCleanerAudioProcessorEditor = AudioPluginAudioProcessorEditor;
