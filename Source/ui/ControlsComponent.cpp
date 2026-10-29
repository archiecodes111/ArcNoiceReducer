#include "ControlsComponent.h"
#include "CustomLookAndFeel.h"

ControlsComponent::ControlsComponent(DenoiseEngine& engineRef)
    : engine(engineRef)
{
    // ==========================================
    // MODULE 1: DSP KNOBS
    // ==========================================
    dspHeader.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    dspHeader.setColour(juce::Label::textColourId, CustomLookAndFeel::textSecondary);
    addAndMakeVisible(dspHeader);

    // Knob 1: Subtraction (Alpha)
    alphaSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    alphaSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    alphaSlider.setRange(0.0, 5.0, 0.05);
    alphaSlider.setValue(engine.getAlpha());
    alphaSlider.onValueChange = [this]
    {
        const float val = static_cast<float>(alphaSlider.getValue());
        alphaValueLabel.setText(juce::String(val, 2), juce::dontSendNotification);
        if (onAlphaChanged) onAlphaChanged(val);
    };
    alphaSlider.onDragEnd = [this]
    {
        if (onSettingsDragEnded) onSettingsDragEnded();
    };
    addAndMakeVisible(alphaSlider);

    alphaLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    alphaLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textDark);
    alphaLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(alphaLabel);

    alphaValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    alphaValueLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::arcCyan);
    alphaValueLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(alphaValueLabel);

    // Knob 2: FFT Size
    fftSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    fftSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    fftSlider.setRange(10, 12, 1);
    fftSlider.setValue(11);
    fftSlider.onValueChange = [this]
    {
        const int order = static_cast<int>(fftSlider.getValue());
        const int size = 1 << order;
        fftValueLabel.setText(juce::String(size) + " pts", juce::dontSendNotification);
        if (onFFTSizeChanged) onFFTSizeChanged(order);
    };
    addAndMakeVisible(fftSlider);

    fftLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    fftLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textDark);
    fftLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fftLabel);

    fftValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    fftValueLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::arcCyan);
    fftValueLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(fftValueLabel);

    // Knob 4: Output Trim
    outputTrimSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputTrimSlider.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
    outputTrimSlider.setRange(-12.0, 12.0, 0.5);
    outputTrimSlider.setValue(0.0);
    outputTrimSlider.onValueChange = [this]
    {
        const float val = static_cast<float>(outputTrimSlider.getValue());
        outputTrimValueLabel.setText((val > 0.0f ? "+" : "") + juce::String(val, 1) + " dB", juce::dontSendNotification);
        engine.setOutputTrimDb(val);
    };
    addAndMakeVisible(outputTrimSlider);

    outputTrimLabel.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    outputTrimLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textDark);
    outputTrimLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputTrimLabel);

    outputTrimValueLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    outputTrimValueLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::arcCyan);
    outputTrimValueLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(outputTrimValueLabel);

    // ==========================================
    // MODULE 2: PROFILING & AUDITION
    // ==========================================
    profileHeader.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    profileHeader.setColour(juce::Label::textColourId, CustomLookAndFeel::textSecondary);
    addAndMakeVisible(profileHeader);

    learnNoiseButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::arcAmber.darker(0.35f));
    learnNoiseButton.onClick = [this] { if (onLearnNoiseClicked) onLearnNoiseClicked(); };
    addAndMakeVisible(learnNoiseButton);

    processButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::arcCyan.darker(0.35f));
    processButton.onClick = [this] { if (onProcessClicked) onProcessClicked(); };
    addAndMakeVisible(processButton);

    auditionLabel.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    auditionLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textSecondary);
    addAndMakeVisible(auditionLabel);

    denoisedButton.setClickingTogglesState(false);
    denoisedButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Denoised);
        engine.setAuditionMode(AuditionMode::Denoised);
        setStatusMessage("Audition: Denoised Master Output");
        if (onAuditionModeChanged)
            onAuditionModeChanged(AuditionMode::Denoised);
    };
    addAndMakeVisible(denoisedButton);

    bypassButton.setClickingTogglesState(false);
    bypassButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Bypass);
        engine.setAuditionMode(AuditionMode::Bypass);
        setStatusMessage("Audition: Bypass (Original Input Audio)");
        if (onAuditionModeChanged)
            onAuditionModeChanged(AuditionMode::Bypass);
    };
    addAndMakeVisible(bypassButton);

    deltaButton.setClickingTogglesState(false);
    deltaButton.onClick = [this]
    {
        updateAuditionButtons(AuditionMode::Delta);
        engine.setAuditionMode(AuditionMode::Delta);
        setStatusMessage("Audition: Delta (Extracted Noise Difference)");
        if (onAuditionModeChanged)
            onAuditionModeChanged(AuditionMode::Delta);
    };
    addAndMakeVisible(deltaButton);

    updateAuditionButtons(engine.getAuditionMode());

    // ==========================================
    // MODULE 3: TRANSPORT & FILES
    // ==========================================
    transportHeader.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    transportHeader.setColour(juce::Label::textColourId, CustomLookAndFeel::textSecondary);
    addAndMakeVisible(transportHeader);

    // Transport buttons
    playButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::arcGreen.darker(0.4f));
    playButton.onClick = [this]
    {
        if (engine.getIsPlaying())
        {
            engine.pausePlayback();
            playButton.setButtonText("PLAY");
            playButton.setToggleState(false, juce::dontSendNotification);
        }
        else
        {
            if (engine.getFileManager().hasInputAudio())
            {
                engine.startPlayback();
                playButton.setButtonText("PAUSE");
                playButton.setToggleState(true, juce::dontSendNotification);
            }
            else
            {
                setStatusMessage("Please load an audio file first.", true);
            }
        }
    };
    addAndMakeVisible(playButton);

    stopButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::btnDark);
    stopButton.onClick = [this]
    {
        engine.stopPlayback();
        playButton.setButtonText("PLAY");
        playButton.setToggleState(false, juce::dontSendNotification);
    };
    addAndMakeVisible(stopButton);

    loopButton.setClickingTogglesState(true);
    loopButton.setToggleState(engine.getIsLooping(), juce::dontSendNotification);
    loopButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::btnDark);
    loopButton.onClick = [this]
    {
        const bool state = loopButton.getToggleState();
        loopButton.setButtonText(state ? "LOOP ON" : "LOOP OFF");
        engine.setLooping(state);
    };
    addAndMakeVisible(loopButton);

    // File action buttons
    loadAudioButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::arcCyan.darker(0.35f));
    loadAudioButton.onClick = [this] { if (onLoadAudioClicked) onLoadAudioClicked(); };
    addAndMakeVisible(loadAudioButton);

    loadNoiseButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::btnDark);
    loadNoiseButton.onClick = [this] { if (onLoadNoiseClicked) onLoadNoiseClicked(); };
    addAndMakeVisible(loadNoiseButton);

    exportButton.setColour(juce::TextButton::buttonColourId, CustomLookAndFeel::btnDark);
    exportButton.onClick = [this] { if (onExportClicked) onExportClicked(); };
    addAndMakeVisible(exportButton);

    // Status & Metadata Readouts
    statusLabel.setFont(juce::FontOptions(11.0f, juce::Font::bold));
    statusLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textDark);
    statusLabel.setText("Ready. Load an audio file to begin.", juce::dontSendNotification);
    addAndMakeVisible(statusLabel);

    metadataLabel.setFont(juce::FontOptions(10.5f, juce::Font::plain));
    metadataLabel.setColour(juce::Label::textColourId, CustomLookAndFeel::textSecondary);
    metadataLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(metadataLabel);

    updateMetadataDisplay();
}

void ControlsComponent::updateAuditionButtons(AuditionMode mode)
{
    const bool isDenoised = (mode == AuditionMode::Denoised);
    const bool isBypass = (mode == AuditionMode::Bypass);
    const bool isDelta = (mode == AuditionMode::Delta || mode == AuditionMode::Residual);

    denoisedButton.setColour(juce::TextButton::buttonColourId,
        isDenoised ? CustomLookAndFeel::arcCyan.darker(0.2f) : CustomLookAndFeel::btnDark);
    denoisedButton.setToggleState(isDenoised, juce::dontSendNotification);

    bypassButton.setColour(juce::TextButton::buttonColourId,
        isBypass ? CustomLookAndFeel::arcRed.darker(0.2f) : CustomLookAndFeel::btnDark);
    bypassButton.setToggleState(isBypass, juce::dontSendNotification);

    deltaButton.setColour(juce::TextButton::buttonColourId,
        isDelta ? CustomLookAndFeel::arcAmber.darker(0.2f) : CustomLookAndFeel::btnDark);
    deltaButton.setToggleState(isDelta, juce::dontSendNotification);

    repaint();
}

void ControlsComponent::setStatusMessage(const juce::String& message, bool isError)
{
    statusLabel.setText(message, juce::dontSendNotification);
    statusLabel.setColour(juce::Label::textColourId,
        isError ? CustomLookAndFeel::arcRed : CustomLookAndFeel::textDark);
}

void ControlsComponent::updateMetadataDisplay()
{
    auto& fm = engine.getFileManager();
    if (fm.hasInputAudio())
    {
        const int samples = fm.getInputBuffer().getNumSamples();
        const int channels = fm.getInputBuffer().getNumChannels();
        const double sr = fm.getInputSampleRate();
        const double dur = (sr > 0.0) ? (static_cast<double>(samples) / sr) : 0.0;

        auto meta = juce::String::formatted("%s   |   %.1f kHz   |   %s   |   %.1fs",
            fm.getInputFileName().toRawUTF8(),
            sr / 1000.0,
            (channels == 1 ? "Mono" : (channels == 2 ? "Stereo" : "Multi-ch")),
            dur);

        if (engine.getNoiseProfile().isReady())
        {
            meta += juce::String::formatted("   |   Profile M=%d", engine.getNoiseProfile().getNumFrames());
        }

        metadataLabel.setText(meta, juce::dontSendNotification);
    }
    else
    {
        metadataLabel.setText("No audio file loaded", juce::dontSendNotification);
    }
}

void ControlsComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float cardSpacing = 8.0f;
    const float footerH = 24.0f;
    const float mainH = bounds.getHeight() - footerH - 4.0f;

    // Proportions: DSP Knobs (38%), Profiling/Audition (31%), Transport/Files (31%)
    const float w1 = (bounds.getWidth() - (cardSpacing * 2.0f)) * 0.38f;
    const float w2 = (bounds.getWidth() - (cardSpacing * 2.0f)) * 0.31f;
    const float w3 = bounds.getWidth() - w1 - w2 - (cardSpacing * 2.0f);

    const float widths[] = { w1, w2, w3 };
    float curX = bounds.getX();

    for (int i = 0; i < 3; ++i)
    {
        const auto cardRect = juce::Rectangle<float>(curX, bounds.getY(), widths[i], mainH);

        // Elevated card module
        g.setColour(CustomLookAndFeel::cardBg);
        g.fillRoundedRectangle(cardRect, 5.0f);

        g.setColour(CustomLookAndFeel::cardBorder);
        g.drawRoundedRectangle(cardRect, 5.0f, 1.0f);

        curX += widths[i] + cardSpacing;
    }

    // Bottom footer line
    const float footerY = bounds.getY() + mainH + 2.0f;
    g.setColour(CustomLookAndFeel::cardBorder.withAlpha(0.6f));
    g.drawHorizontalLine(static_cast<int>(footerY), bounds.getX(), bounds.getRight());
}

void ControlsComponent::resized()
{
    const auto bounds = getLocalBounds();
    const int cardSpacing = 8;
    const int footerH = 22;
    const int mainH = bounds.getHeight() - footerH - 4;

    const int totalW = bounds.getWidth() - (cardSpacing * 2);
    const int w1 = static_cast<int>(totalW * 0.38f);
    const int w2 = static_cast<int>(totalW * 0.31f);
    const int w3 = totalW - w1 - w2;

    // ------------------------------------------
    // CARD 1: MASTER DSP CONTROLS (3 Knobs)
    // ------------------------------------------
    auto c1 = juce::Rectangle<int>(bounds.getX(), bounds.getY(), w1, mainH).reduced(8);
    dspHeader.setBounds(c1.removeFromTop(16));
    c1.removeFromTop(4);

    const int knobColW = c1.getWidth() / 3;
    const int knobSize = 60;

    // Knob 1: Subtraction (Alpha)
    {
        auto col = c1.removeFromLeft(knobColW);
        alphaLabel.setBounds(col.removeFromTop(14));
        alphaSlider.setBounds(col.getCentreX() - (knobSize / 2), col.getY() + 2, knobSize, knobSize + 8);
        alphaValueLabel.setBounds(col.getCentreX() - 35, col.getY() + 72, 70, 16);
    }
    // Knob 2: FFT Size
    {
        auto col = c1.removeFromLeft(knobColW);
        fftLabel.setBounds(col.removeFromTop(14));
        fftSlider.setBounds(col.getCentreX() - (knobSize / 2), col.getY() + 2, knobSize, knobSize + 8);
        fftValueLabel.setBounds(col.getCentreX() - 35, col.getY() + 72, 70, 16);
    }
    // Knob 3: Output Trim
    {
        auto col = c1;
        outputTrimLabel.setBounds(col.removeFromTop(14));
        outputTrimSlider.setBounds(col.getCentreX() - (knobSize / 2), col.getY() + 2, knobSize, knobSize + 8);
        outputTrimValueLabel.setBounds(col.getCentreX() - 35, col.getY() + 72, 70, 16);
    }

    // ------------------------------------------
    // CARD 2: PROFILING & AUDITION
    // ------------------------------------------
    const int c2X = bounds.getX() + w1 + cardSpacing;
    auto c2 = juce::Rectangle<int>(c2X, bounds.getY(), w2, mainH).reduced(10);
    profileHeader.setBounds(c2.removeFromTop(16));
    c2.removeFromTop(8);

    learnNoiseButton.setBounds(c2.removeFromTop(32));
    c2.removeFromTop(6);

    processButton.setBounds(c2.removeFromTop(34));
    c2.removeFromTop(10);

    auditionLabel.setBounds(c2.removeFromTop(14));
    c2.removeFromTop(4);

    auto auditRow = c2.removeFromTop(32);
    const int auditBtnW = (auditRow.getWidth() - 6) / 3;
    denoisedButton.setBounds(auditRow.removeFromLeft(auditBtnW));
    auditRow.removeFromLeft(3);
    bypassButton.setBounds(auditRow.removeFromLeft(auditBtnW));
    auditRow.removeFromLeft(3);
    deltaButton.setBounds(auditRow);

    // ------------------------------------------
    // CARD 3: TRANSPORT & FILES
    // ------------------------------------------
    const int c3X = c2X + w2 + cardSpacing;
    auto c3 = juce::Rectangle<int>(c3X, bounds.getY(), w3, mainH).reduced(10);
    transportHeader.setBounds(c3.removeFromTop(16));
    c3.removeFromTop(12);

    // Transport row
    auto transRow = c3.removeFromTop(38);
    const int transBtnW = (transRow.getWidth() - 6) / 3;
    playButton.setBounds(transRow.removeFromLeft(transBtnW));
    transRow.removeFromLeft(3);
    stopButton.setBounds(transRow.removeFromLeft(transBtnW));
    transRow.removeFromLeft(3);
    loopButton.setBounds(transRow);

    c3.removeFromTop(20);

    // File buttons row
    auto fileRow = c3.removeFromTop(36);
    const int fileBtnW = (fileRow.getWidth() - 6) / 3;
    loadAudioButton.setBounds(fileRow.removeFromLeft(fileBtnW));
    fileRow.removeFromLeft(3);
    loadNoiseButton.setBounds(fileRow.removeFromLeft(fileBtnW));
    fileRow.removeFromLeft(3);
    exportButton.setBounds(fileRow);

    // ------------------------------------------
    // FOOTER: Status and Metadata
    // ------------------------------------------
    const int footerY = bounds.getY() + mainH + 3;
    statusLabel.setBounds(bounds.getX() + 8, footerY, 440, 18);
    metadataLabel.setBounds(bounds.getRight() - 448, footerY, 440, 18);
}
