#include "WaveformComponent.h"
#include "CustomLookAndFeel.h"
#include <cmath>

WaveformComponent::WaveformComponent()
{
    startTimerHz(30);
}

WaveformComponent::~WaveformComponent()
{
    stopTimer();
}

void WaveformComponent::setAudioBuffer(const juce::AudioBuffer<float>* buffer, double sr)
{
    audioBuffer = buffer;
    sampleRate = (sr > 0.0) ? sr : 44100.0;
    totalSamples = (audioBuffer != nullptr) ? audioBuffer->getNumSamples() : 0;

    if (totalSamples > 0)
    {
        const int defaultNoiseSamples = std::min(totalSamples, static_cast<int>(sampleRate * 0.75));
        setSelectionRange(0, defaultNoiseSamples);
    }
    else
    {
        setSelectionRange(0, 0);
    }

    currentPlayheadSample = 0;
    regenerateWaveformPath();
    repaint();
}

void WaveformComponent::setPlayheadSample(int sampleIndex)
{
    currentPlayheadSample = sampleIndex;
}

void WaveformComponent::setSelectionRange(int startSample, int numSamples)
{
    if (totalSamples <= 0)
    {
        selectionStartSample = 0;
        selectionNumSamples = 0;
    }
    else
    {
        selectionStartSample = juce::jlimit(0, totalSamples - 1, startSample);
        selectionNumSamples = juce::jlimit(0, totalSamples - selectionStartSample, numSamples);
    }
    repaint();
}

void WaveformComponent::timerCallback()
{
    repaint();
}

void WaveformComponent::resized()
{
    regenerateWaveformPath();
}

int WaveformComponent::xCoordinateToSample(float x) const noexcept
{
    if (totalSamples <= 0 || getWidth() <= 0)
        return 0;

    const float normalizedX = juce::jlimit(0.0f, 1.0f, x / static_cast<float>(getWidth()));
    return static_cast<int>(normalizedX * static_cast<float>(totalSamples));
}

float WaveformComponent::sampleToXCoordinate(int sample) const noexcept
{
    if (totalSamples <= 0 || getWidth() <= 0)
        return 0.0f;

    const float normalizedSample = static_cast<float>(sample) / static_cast<float>(totalSamples);
    return normalizedSample * static_cast<float>(getWidth());
}

void WaveformComponent::regenerateWaveformPath()
{
    waveformPath.clear();
    envelopePath.clear();

    const int width = getWidth();
    const int height = getHeight();

    if (audioBuffer == nullptr || totalSamples <= 0 || width <= 0 || height <= 0)
        return;

    const int numChannels = audioBuffer->getNumChannels();
    if (numChannels <= 0)
        return;
    const float rulerH = 16.0f;
    const float waveAreaH = static_cast<float>(height) - rulerH - 2.0f;
    const float midY = rulerH + (waveAreaH * 0.5f);
    const float halfHeight = waveAreaH * 0.46f;

    const int samplesPerPixel = std::max(1, totalSamples / width);

    juce::Path topPath;
    juce::Path bottomPath;

    for (int pixelX = 0; pixelX < width; ++pixelX)
    {
        const int startSample = pixelX * samplesPerPixel;
        const int endSample = std::min(totalSamples, startSample + samplesPerPixel);

        float minVal = 0.0f;
        float maxVal = 0.0f;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = audioBuffer->getReadPointer(ch);
            for (int s = startSample; s < endSample; ++s)
            {
                const float val = channelData[s];
                if (val < minVal) minVal = val;
                if (val > maxVal) maxVal = val;
            }
        }

        const float topY = midY - (juce::jlimit(0.0f, 1.0f, maxVal) * halfHeight);
        const float botY = midY - (juce::jlimit(-1.0f, 0.0f, minVal) * halfHeight);
        const float x = static_cast<float>(pixelX);

        if (pixelX == 0)
        {
            topPath.startNewSubPath(x, topY);
            bottomPath.startNewSubPath(x, botY);
        }
        else
        {
            topPath.lineTo(x, topY);
            bottomPath.lineTo(x, botY);
        }
    }

    waveformPath = topPath;
    waveformPath.addPath(bottomPath);

    // RMS envelope fill
    envelopePath = topPath;
    for (int pixelX = width - 1; pixelX >= 0; --pixelX)
    {
        const int startSample = pixelX * samplesPerPixel;
        const int endSample = std::min(totalSamples, startSample + samplesPerPixel);

        float minVal = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = audioBuffer->getReadPointer(ch);
            for (int s = startSample; s < endSample; ++s)
            {
                const float val = channelData[s];
                if (val < minVal) minVal = val;
            }
        }
        const float botY = midY - (juce::jlimit(-1.0f, 0.0f, minVal) * halfHeight);
        envelopePath.lineTo(static_cast<float>(pixelX), botY);
    }
    envelopePath.closeSubPath();
}

void WaveformComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Weiss light display surface
    g.setColour(juce::Colour(0xfff7f2f2));
    g.fillRect(bounds);

    if (totalSamples <= 0)
    {
        const float cx = w * 0.5f;
        const float cy = h * 0.44f;

        g.setColour(juce::Colour(0xffdcd3d3));
        const float barHeights[] = { 8.0f, 20.0f, 32.0f, 24.0f, 12.0f, 28.0f, 16.0f };
        for (int i = 0; i < 7; ++i)
        {
            const float bx = cx - 36.0f + (i * 12.0f);
            const float bh = barHeights[i];
            g.fillRoundedRectangle(bx, cy - bh * 0.5f, 4.0f, bh, 2.0f);
        }

        g.setColour(CustomLookAndFeel::textMuted);
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText("Load an audio file to view waveform & select noise segment",
                   0, static_cast<int>(cy + 24.0f), static_cast<int>(w), 20, juce::Justification::centred);
        return;
    }

    // 1. Time ruler across top
    const float rulerH = 16.0f;
    g.setColour(juce::Colour(0xffeee7e7));
    g.fillRect(0.0f, 0.0f, w, rulerH);

    g.setColour(juce::Colour(0xffe0d5d5));
    g.drawHorizontalLine(static_cast<int>(rulerH), 0.0f, w);

    const double totalSec = static_cast<double>(totalSamples) / sampleRate;
    double tickInterval = 1.0;
    if (totalSec > 60.0) tickInterval = 10.0;
    else if (totalSec > 20.0) tickInterval = 5.0;
    else if (totalSec > 5.0) tickInterval = 1.0;
    else tickInterval = 0.5;

    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));
    g.setColour(juce::Colour(0xff8a7d80));

    for (double t = 0.0; t <= totalSec; t += tickInterval)
    {
        const int s = static_cast<int>(t * sampleRate);
        const float x = sampleToXCoordinate(s);

        g.drawVerticalLine(static_cast<int>(x), 8.0f, rulerH);

        const auto timeStr = juce::String::formatted("%.1fs", t);
        g.drawText(timeStr, static_cast<int>(x) + 2, 0, 36, static_cast<int>(rulerH), juce::Justification::centredLeft);
    }

    // 2. Center zero line
    const float waveAreaH = h - rulerH - 2.0f;
    const float midY = rulerH + (waveAreaH * 0.5f);

    g.setColour(juce::Colour(0xffe8dcdd));
    g.drawHorizontalLine(static_cast<int>(midY), 0.0f, w);

    // 3. Waveform fill (Soft slate blue-gray)
    juce::ColourGradient waveGrad(juce::Colour(0xff5c6b73).withAlpha(0.35f), 0.0f, midY - 20.0f,
                                  juce::Colour(0xff5c6b73).withAlpha(0.08f), 0.0f, midY + 20.0f, false);
    g.setGradientFill(waveGrad);
    g.fillPath(envelopePath);

    // 4. Waveform Peak line (Matte charcoal)
    g.setColour(juce::Colour(0xff2d3748));
    g.strokePath(waveformPath, juce::PathStrokeType(1.0f));

    // 5. Draw Noise Selection Region (ARC Amber overlay)
    if (selectionNumSamples > 0)
    {
        const float selX = sampleToXCoordinate(selectionStartSample);
        const float selW = std::max(2.0f, sampleToXCoordinate(selectionStartSample + selectionNumSamples) - selX);
        const auto selBounds = juce::Rectangle<float>(selX, rulerH, selW, h - rulerH);

        // Soft amber tint
        g.setColour(CustomLookAndFeel::arcAmber.withAlpha(0.18f));
        g.fillRect(selBounds);

        // Vertical boundary lines
        g.setColour(CustomLookAndFeel::arcAmber);
        g.drawVerticalLine(static_cast<int>(selX), rulerH, h);
        g.drawVerticalLine(static_cast<int>(selX + selW), rulerH, h);

        // Handle dots
        g.fillEllipse(selX - 3.0f, midY - 3.0f, 6.0f, 6.0f);
        g.fillEllipse(selX + selW - 3.0f, midY - 3.0f, 6.0f, 6.0f);

        // Badge banner
        const double startSec = static_cast<double>(selectionStartSample) / sampleRate;
        const double durSec = static_cast<double>(selectionNumSamples) / sampleRate;
        const auto badgeText = juce::String::formatted("NOISE PROFILE: %.2fs - %.2fs (%.2fs)", startSec, startSec + durSec, durSec);

        g.setFont(juce::FontOptions(9.0f, juce::Font::bold));
        const int badgeW = 210;
        const auto badgeBounds = juce::Rectangle<float>(juce::jlimit(4.0f, w - badgeW - 4.0f, selX + 4.0f),
                                                        rulerH + 3.0f, static_cast<float>(badgeW), 16.0f);

        g.setColour(juce::Colours::white.withAlpha(0.95f));
        g.fillRoundedRectangle(badgeBounds, 2.0f);
        g.setColour(CustomLookAndFeel::arcAmber);
        g.drawRoundedRectangle(badgeBounds, 2.0f, 1.0f);
        g.setColour(CustomLookAndFeel::textDark);
        g.drawText(badgeText, badgeBounds, juce::Justification::centred);
    }

    // 6. Draw ARC Cyan Playhead Needle
    if (totalSamples > 0)
    {
        const float playX = sampleToXCoordinate(currentPlayheadSample);

        g.setColour(CustomLookAndFeel::arcCyan);
        g.drawLine(playX, rulerH, playX, h, 1.5f);

        juce::Path head;
        head.addTriangle(playX - 3.5f, rulerH, playX + 3.5f, rulerH, playX, rulerH + 5.0f);
        g.fillPath(head);
    }
}

void WaveformComponent::mouseMove(const juce::MouseEvent& e)
{
    if (totalSamples <= 0)
    {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        return;
    }

    const float selX1 = sampleToXCoordinate(selectionStartSample);
    const float selX2 = sampleToXCoordinate(selectionStartSample + selectionNumSamples);
    const float x = static_cast<float>(e.x);

    if (std::abs(x - selX1) < 6.0f || std::abs(x - selX2) < 6.0f)
        setMouseCursor(juce::MouseCursor::LeftRightResizeCursor);
    else if (x >= selX1 && x <= selX2)
        setMouseCursor(juce::MouseCursor::PointingHandCursor);
    else
        setMouseCursor(juce::MouseCursor::CrosshairCursor);
}

void WaveformComponent::mouseDown(const juce::MouseEvent& e)
{
    if (totalSamples <= 0)
        return;

    if (e.mods.isShiftDown() || e.mods.isRightButtonDown())
    {
        isScrubbing = true;
        const int targetSample = xCoordinateToSample(static_cast<float>(e.x));
        if (onPlayheadScrubbed)
            onPlayheadScrubbed(targetSample);
    }
    else
    {
        isDraggingSelection = true;
        selectionStartSample = xCoordinateToSample(static_cast<float>(e.x));
        selectionNumSamples = 0;
        repaint();
    }
}

void WaveformComponent::mouseDrag(const juce::MouseEvent& e)
{
    if (totalSamples <= 0)
        return;

    if (isScrubbing)
    {
        const int targetSample = xCoordinateToSample(static_cast<float>(e.x));
        if (onPlayheadScrubbed)
            onPlayheadScrubbed(targetSample);
    }
    else if (isDraggingSelection)
    {
        const int currentSample = xCoordinateToSample(static_cast<float>(e.x));
        if (currentSample >= selectionStartSample)
        {
            selectionNumSamples = currentSample - selectionStartSample;
        }
        else
        {
            selectionNumSamples = selectionStartSample - currentSample;
            selectionStartSample = currentSample;
        }
        repaint();
    }
}

void WaveformComponent::mouseUp(const juce::MouseEvent& e)
{
    juce::ignoreUnused(e);

    if (isDraggingSelection)
    {
        isDraggingSelection = false;

        if (selectionNumSamples < static_cast<int>(sampleRate * 0.05))
        {
            const int defaultLen = static_cast<int>(sampleRate * 0.5);
            selectionNumSamples = std::min(defaultLen, totalSamples - selectionStartSample);
        }

        if (onNoiseRegionSelected)
            onNoiseRegionSelected(selectionStartSample, selectionNumSamples);

        repaint();
    }

    isScrubbing = false;
}
