#include "TransportComponent.h"
#include "CustomLookAndFeel.h"
#include <cmath>

TransportComponent::TransportComponent(DenoiseEngine& engineRef)
    : engine(engineRef)
{
    // Play/Pause button
    playPauseButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::bgElevated);
    playPauseButton.setColour(juce::TextButton::textColourOffId, CustomLookAndFeel::accentGreen);
    playPauseButton.onClick = [this]
    {
        if (engine.getIsPlaying())
        {
            if (onPauseClicked) onPauseClicked();
        }
        else
        {
            if (onPlayClicked) onPlayClicked();
        }
    };
    addAndMakeVisible(playPauseButton);

    // Stop button
    stopButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::bgElevated);
    stopButton.setColour(juce::TextButton::textColourOffId, CustomLookAndFeel::textSecondary);
    stopButton.onClick = [this]
    {
        if (onStopClicked) onStopClicked();
    };
    addAndMakeVisible(stopButton);

    // Loop button
    loopButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::bgElevated);
    loopButton.setClickingTogglesState(true);
    loopButton.setToggleState(engine.getIsLooping(), juce::dontSendNotification);
    loopButton.setButtonText(loopButton.getToggleState() ? "Loop ON" : "Loop OFF");
    loopButton.onClick = [this]
    {
        const bool state = loopButton.getToggleState();
        loopButton.setButtonText(state ? "Loop ON" : "Loop OFF");
        if (onLoopToggled) onLoopToggled(state);
    };
    addAndMakeVisible(loopButton);

    // Audition section label
    auditionLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    auditionLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textMuted);
    auditionLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(auditionLabel);

    // Segmented Audition Buttons
    denoisedButton.setClickingTogglesState(false);
    denoisedButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Denoised);
        if (onAuditionModeChanged) onAuditionModeChanged(AuditionMode::Denoised);
    };
    addAndMakeVisible(denoisedButton);

    bypassButton.setClickingTogglesState(false);
    bypassButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Bypass);
        if (onAuditionModeChanged) onAuditionModeChanged(AuditionMode::Bypass);
    };
    addAndMakeVisible(bypassButton);

    residualButton.setClickingTogglesState(false);
    residualButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Residual);
        if (onAuditionModeChanged) onAuditionModeChanged(AuditionMode::Residual);
    };
    addAndMakeVisible(residualButton);

    updateAuditionButtons(engine.getAuditionMode());

    // Timecode LCD label
    timeLabel.setFont(juce::FontOptions(12.0f, juce::Font::plain));
    timeLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::accentCyan);
    timeLabel.setColour(juce::Label::backgroundColourId, CustomLookAndFeel::bgDeep);
    timeLabel.setColour(juce::Label::outlineColourId, CustomLookAndFeel::borderSubtle);
    timeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(timeLabel);

    startTimerHz(30);
}

TransportComponent::~TransportComponent()
{
    stopTimer();
}

void TransportComponent::updateAuditionButtons(AuditionMode activeMode)
{
    denoisedButton.setColour(juce::TextButton::buttonColourId,
        (activeMode == AuditionMode::Denoised) ? CustomLookAndFeel::accentCyan.withAlpha(0.25f) : CustomLookAndFeel::bgElevated);
    denoisedButton.setColour(juce::TextButton::textColourOffId,
        (activeMode == AuditionMode::Denoised) ? CustomLookAndFeel::accentCyan : CustomLookAndFeel::textSecondary);

    bypassButton.setColour(juce::TextButton::buttonColourId,
        (activeMode == AuditionMode::Bypass) ? CustomLookAndFeel::textSecondary.withAlpha(0.25f) : CustomLookAndFeel::bgElevated);
    bypassButton.setColour(juce::TextButton::textColourOffId,
        (activeMode == AuditionMode::Bypass) ? CustomLookAndFeel::textPrimary : CustomLookAndFeel::textSecondary);

    residualButton.setColour(juce::TextButton::buttonColourId,
        (activeMode == AuditionMode::Residual) ? CustomLookAndFeel::accentAmber.withAlpha(0.25f) : CustomLookAndFeel::bgElevated);
    residualButton.setColour(juce::TextButton::textColourOffId,
        (activeMode == AuditionMode::Residual) ? CustomLookAndFeel::accentAmber : CustomLookAndFeel::textSecondary);
}

void TransportComponent::timerCallback()
{
    const bool playing = engine.getIsPlaying();
    playPauseButton.setButtonText(playing ? "Pause" : "Play");
    playPauseButton.setColour(juce::TextButton::textColourOffId,
        playing ? CustomLookAndFeel::accentAmber : CustomLookAndFeel::accentGreen);

    const double sr = engine.getSampleRate();
    const int currentSample = engine.getPlayheadSample();
    const int totalSamples = engine.getInputBuffer().getNumSamples();

    if (sr > 0.0 && totalSamples > 0)
    {
        const double curSec = static_cast<double>(currentSample) / sr;
        const double totSec = static_cast<double>(totalSamples) / sr;

        const int curMin = static_cast<int>(curSec) / 60;
        const double curRemSec = std::fmod(curSec, 60.0);

        const int totMin = static_cast<int>(totSec) / 60;
        const double totRemSec = std::fmod(totSec, 60.0);

        timeLabel.setText(juce::String::formatted("%02d:%04.1f / %02d:%04.1f", curMin, curRemSec, totMin, totRemSec),
                          juce::dontSendNotification);
    }
    else
    {
        timeLabel.setText("00:00.0 / 00:00.0", juce::dontSendNotification);
    }
}

void TransportComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour(CustomLookAndFeel::bgPanel);
    g.fillRoundedRectangle(bounds, 8.0f);
    g.setColour(CustomLookAndFeel::borderSubtle);
    g.drawRoundedRectangle(bounds, 8.0f, 1.0f);
}

void TransportComponent::resized()
{
    auto area = getLocalBounds().reduced(8);

    // Transport buttons on left
    playPauseButton.setBounds(area.removeFromLeft(68));
    area.removeFromLeft(6);
    stopButton.setBounds(area.removeFromLeft(56));
    area.removeFromLeft(6);
    loopButton.setBounds(area.removeFromLeft(70));

    area.removeFromLeft(20);

    // Segmented audition switcher in middle
    auditionLabel.setBounds(area.removeFromLeft(90));
    area.removeFromLeft(6);
    denoisedButton.setBounds(area.removeFromLeft(82));
    area.removeFromLeft(4);
    bypassButton.setBounds(area.removeFromLeft(74));
    area.removeFromLeft(4);
    residualButton.setBounds(area.removeFromLeft(110));

    // Timecode LCD on far right
    timeLabel.setBounds(area.removeFromRight(150));
}
