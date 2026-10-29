#include "PluginEditor.h"
#include <cmath>

AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor(AudioPluginAudioProcessor& p)
    : AudioProcessorEditor(&p),
      processorRef(p),
      displayComponent(p.getDenoiseEngine()),
      controlsComponent(p.getDenoiseEngine())
{
    juce::LookAndFeel::setDefaultLookAndFeel(&customLookAndFeel);

    addAndMakeVisible(displayComponent);
    addAndMakeVisible(controlsComponent);

    // Wire APVTS slider attachments
    alphaAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(
        processorRef.apvts, "alpha", controlsComponent.getAlphaSlider());

    // Wire Waveform Callbacks inside the Weiss Display
    auto& waveform = displayComponent.getWaveformComponent();
    waveform.onPlayheadScrubbed = [this](int sample)
    {
        processorRef.getDenoiseEngine().seekToSample(sample);
    };

    waveform.onNoiseRegionSelected = [this](int startSample, int numSamples)
    {
        auto& engine = processorRef.getDenoiseEngine();
        controlsComponent.setStatusMessage(juce::String::formatted(
            "Noise segment marked (%.2fs - %.2fs). Click 'learn' to capture profile.",
            static_cast<double>(startSample) / engine.getSampleRate(),
            static_cast<double>(startSample + numSamples) / engine.getSampleRate()));
    };

    // Wire Controls Callbacks
    controlsComponent.onLoadAudioClicked = [this]
    {
        openAudioFileChooser();
    };

    controlsComponent.onLoadNoiseClicked = [this]
    {
        openNoiseFileChooser();
    };

    controlsComponent.onExportClicked = [this]
    {
        openExportFileChooser();
    };

    controlsComponent.onLearnNoiseClicked = [this]
    {
        auto& engine = processorRef.getDenoiseEngine();
        auto& wf = displayComponent.getWaveformComponent();
        const int selStart = wf.getSelectionStartSample();
        const int selCount = wf.getSelectionNumSamples();

        if (selCount > engine.getFFTSize())
        {
            controlsComponent.setStatusMessage("Estimating noise profile from selection...");
            if (engine.estimateNoiseFromSelection(selStart, selCount))
            {
                controlsComponent.setStatusMessage(juce::String::formatted(
                    "Noise profile captured (M = %d frames). Click '2. PROCESS AUDIO' to apply.",
                    engine.getNoiseProfile().getNumFrames()));
                controlsComponent.updateMetadataDisplay();
            }
            else
            {
                controlsComponent.setStatusMessage("Failed to estimate noise profile.", true);
            }
        }
        else
        {
            controlsComponent.setStatusMessage("Select a noise segment on the waveform first.", true);
        }
    };

    controlsComponent.onProcessClicked = [this]
    {
        auto& engine = processorRef.getDenoiseEngine();
        if (!engine.getFileManager().hasInputAudio())
        {
            controlsComponent.setStatusMessage("Please load an audio file first.", true);
            return;
        }

        if (!engine.getNoiseProfile().isReady())
        {
            controlsComponent.setStatusMessage("Please capture a noise profile first (click '1. CAPTURE PROFILE').", true);
            return;
        }

        controlsComponent.setStatusMessage("Processing noise reduction in real-time...");
        engine.processAudio();
        controlsComponent.setStatusMessage(juce::String::formatted(
            "Noise reduction active! (Alpha=%.2f, FFT=%d). Real-time active.",
            engine.getAlpha(), engine.getFFTSize()));
        controlsComponent.updateMetadataDisplay();
    };

    controlsComponent.onAuditionModeChanged = [this](AuditionMode mode)
    {
        if (auto* modeParam = dynamic_cast<juce::AudioParameterChoice*>(processorRef.apvts.getParameter("auditionMode")))
        {
            *modeParam = static_cast<int>(mode);
        }
    };

    controlsComponent.onSettingsDragEnded = [this]
    {
        auto& engine = processorRef.getDenoiseEngine();
        if (engine.hasProcessedAudio())
        {
            engine.processAudio();
            controlsComponent.setStatusMessage(juce::String::formatted(
                "Updated in real-time! (Alpha=%.2f, FFT=%d)",
                engine.getAlpha(), engine.getFFTSize()));
            controlsComponent.updateMetadataDisplay();
        }
    };

    controlsComponent.onAlphaChanged = [this](float val)
    {
        auto& engine = processorRef.getDenoiseEngine();
        engine.setAlpha(val);
        if (engine.hasProcessedAudio())
            controlsComponent.setStatusMessage("Subtraction set to " + juce::String(val, 2) + ". Click '2. PROCESS AUDIO' to update in real-time.");
    };

    controlsComponent.onFFTSizeChanged = [this](int order)
    {
        auto& engine = processorRef.getDenoiseEngine();
        engine.setFFTOrder(order);
        if (engine.hasProcessedAudio())
        {
            controlsComponent.setStatusMessage(juce::String::formatted(
                "FFT Size set to %d pts. Audio and spectrum updated in real-time.", engine.getFFTSize()));
        }
        else
        {
            controlsComponent.setStatusMessage(juce::String::formatted(
                "FFT Size set to %d pts. Click '2. PROCESS AUDIO' to apply.", engine.getFFTSize()));
        }
    };

    setSize(960, 700);
    startTimerHz(30);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
    stopTimer();
    juce::LookAndFeel::setDefaultLookAndFeel(nullptr);
}

void AudioPluginAudioProcessorEditor::drawArcLogo(juce::Graphics& g, float x, float y, float size)
{
    g.saveState();
    const float strokeW = size * 0.12f;
    const float cx = x + size * 0.5f;
    const float cy = y + size * 0.5f;

    // Outer acoustic filtration arc (vibrant electric cyan)
    juce::Path outerArc;
    outerArc.addCentredArc(cx, cy, size * 0.44f, size * 0.44f, 0.0f,
                           -juce::MathConstants<float>::pi * 0.70f,
                           juce::MathConstants<float>::pi * 0.70f, true);
    g.setColour(CustomLookAndFeel::arcCyan);
    g.strokePath(outerArc, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Inner acoustic filtration arc (deep precision slate)
    juce::Path innerArc;
    innerArc.addCentredArc(cx, cy, size * 0.25f, size * 0.25f, 0.0f,
                           -juce::MathConstants<float>::pi * 0.70f,
                           juce::MathConstants<float>::pi * 0.70f, true);
    g.setColour(CustomLookAndFeel::textDark);
    g.strokePath(innerArc, juce::PathStrokeType(strokeW, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // Center focal point
    g.setColour(CustomLookAndFeel::arcCyan);
    g.fillEllipse(cx - strokeW * 0.55f, cy - strokeW * 0.55f, strokeW * 1.1f, strokeW * 1.1f);

    g.restoreState();
}

bool AudioPluginAudioProcessorEditor::isInterestedInFileDrag(const juce::StringArray& files)
{
    for (auto& f : files)
    {
        if (f.endsWithIgnoreCase(".wav") || f.endsWithIgnoreCase(".mp3")
         || f.endsWithIgnoreCase(".aif") || f.endsWithIgnoreCase(".aiff")
         || f.endsWithIgnoreCase(".flac") || f.endsWithIgnoreCase(".ogg"))
            return true;
    }
    return false;
}

void AudioPluginAudioProcessorEditor::filesDropped(const juce::StringArray& files, int x, int y)
{
    juce::ignoreUnused(x, y);
    if (files.size() > 0)
    {
        const juce::File file(files[0]);
        loadFile(file);
    }
}

void AudioPluginAudioProcessorEditor::loadFile(const juce::File& file)
{
    if (file.existsAsFile())
    {
        juce::String error;
        auto& engine = processorRef.getDenoiseEngine();

        if (engine.loadInputFile(file, error))
        {
            displayComponent.getWaveformComponent().setAudioBuffer(&engine.getInputBuffer(), engine.getSampleRate());
            controlsComponent.setStatusMessage("Loaded: " + file.getFileName() + " - Select noise on waveform, then click '1. CAPTURE PROFILE'");
            controlsComponent.updateMetadataDisplay();
        }
        else
        {
            controlsComponent.setStatusMessage("Load error: " + error, true);
        }
    }
}

void AudioPluginAudioProcessorEditor::openAudioFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Audio File with Background Noise",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.mp3;*.aif;*.aiff;*.flac;*.ogg");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        loadFile(file);
    });
}

void AudioPluginAudioProcessorEditor::openNoiseFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Select Dedicated Noise-Only Audio File",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory),
        "*.wav;*.mp3;*.aif;*.aiff;*.flac;*.ogg");

    const auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file.existsAsFile())
        {
            juce::String error;
            auto& engine = processorRef.getDenoiseEngine();

            if (engine.loadNoiseFile(file, error))
            {
                controlsComponent.setStatusMessage(juce::String::formatted(
                    "Noise profile loaded from %s (M = %d frames).",
                    file.getFileName().toRawUTF8(),
                    engine.getNoiseProfile().getNumFrames()));
                controlsComponent.updateMetadataDisplay();
            }
            else
            {
                controlsComponent.setStatusMessage("Noise load error: " + error, true);
            }
        }
    });
}

void AudioPluginAudioProcessorEditor::openExportFileChooser()
{
    fileChooser = std::make_unique<juce::FileChooser>(
        "Export Cleaned Denoised Audio",
        juce::File::getSpecialLocation(juce::File::userHomeDirectory).getChildFile("denoised_master.wav"),
        "*.wav");

    const auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting;

    fileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
    {
        const auto file = fc.getResult();
        if (file != juce::File{})
        {
            juce::String error;
            auto& engine = processorRef.getDenoiseEngine();

            if (engine.exportProcessedFile(file, 24, error))
            {
                controlsComponent.setStatusMessage("Exported master to: " + file.getFileName());
            }
            else
            {
                controlsComponent.setStatusMessage("Export failed: " + error, true);
            }
        }
    });
}

void AudioPluginAudioProcessorEditor::timerCallback()
{
    auto& engine = processorRef.getDenoiseEngine();

    // 1. Update waveform playhead
    displayComponent.getWaveformComponent().setPlayheadSample(engine.getPlayheadSample());

    // 2. Update spectrum curves
    std::vector<float> inMag, nMag, outMag;
    engine.getSpectrumSnapshot(inMag, nMag, outMag);
    displayComponent.getSpectrumComponent().updateSpectra(inMag, nMag, outMag, engine.getSampleRate());

    // 3. Update output meter levels on display flanks
    const float rawLeft = processorRef.getRmsLevel(0);
    const float rawRight = processorRef.getRmsLevel(1);
    displayComponent.setOutputLevels(rawLeft, rawRight);
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();

    // 1. Warm brushed-aluminum chassis surface
    g.setColour(CustomLookAndFeel::chassisBg);
    g.fillRect(bounds);

    // 2. Top Dark Utility Toolbar
    const float topBarH = 26.0f;
    g.setColour(CustomLookAndFeel::topBarBg);
    g.fillRect(0.0f, 0.0f, w, topBarH);

    // Left toolbar preset browser text
    g.setColour(juce::Colour(0xff94a3b8));
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));
    g.drawText("<  >  Preset: Default Studio Subtraction  v", 14, 0, 240, static_cast<int>(topBarH), juce::Justification::centredLeft);

    // Right toolbar ARC DSP brand pill badge
    const auto badgeRect = juce::Rectangle<float>(w - 82.0f, 4.0f, 70.0f, 18.0f);
    g.setColour(CustomLookAndFeel::arcCyan);
    g.fillRoundedRectangle(badgeRect, 3.0f);
    g.setColour(juce::Colours::white);
    g.setFont(juce::FontOptions(9.5f, juce::Font::bold));
    g.drawText("ARC DSP", badgeRect, juce::Justification::centred);

    // 3. Main Header Area (below toolbar)
    const float headerY = topBarH + 11.0f;

    // Draw ARC acoustic arc logo
    drawArcLogo(g, 18.0f, headerY, 24.0f);

    // Draw Brand Text: ARC in Electric Cyan, NOISE REDUCER in Deep Slate
    const float textX = 52.0f;
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.setColour(CustomLookAndFeel::arcCyan);
    g.drawText("ARC", static_cast<int>(textX), static_cast<int>(headerY) - 1, 46, 26, juce::Justification::left);

    g.setColour(CustomLookAndFeel::textDark);
    g.drawText("NOISE REDUCER", static_cast<int>(textX + 44.0f), static_cast<int>(headerY) - 1, 180, 26, juce::Justification::left);

    // Professional subtitle on far right
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.setColour(CustomLookAndFeel::textSecondary);
    g.drawText("SPECTRAL SUBTRACTION MASTERING SUITE", static_cast<int>(w - 290.0f), static_cast<int>(headerY) + 3, 274, 18, juce::Justification::right);

    // Hairline divider
    g.setColour(CustomLookAndFeel::chassisBorder);
    g.drawHorizontalLine(static_cast<int>(headerY + 30.0f), 0.0f, w);
}

void AudioPluginAudioProcessorEditor::resized()
{
    const auto bounds = getLocalBounds();
    const int padX = 14;

    const int topBarH = 26;
    const int headerH = 42;
    const int displayTop = topBarH + headerH + 6;

    // Center Display Frame (Recessed Mastering Display)
    const int displayH = 360;
    displayComponent.setBounds(padX, displayTop, bounds.getWidth() - (padX * 2), displayH);

    // Lower Control Deck (Knobs & Buttons)
    const int controlsTop = displayTop + displayH + 8;
    const int controlsH = bounds.getHeight() - controlsTop - 10;
    controlsComponent.setBounds(padX, controlsTop, bounds.getWidth() - (padX * 2), controlsH);
}
