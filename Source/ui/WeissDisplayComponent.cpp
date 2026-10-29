#include "WeissDisplayComponent.h"
#include "CustomLookAndFeel.h"
#include <cmath>

WeissDisplayComponent::WeissDisplayComponent(DenoiseEngine& engineRef)
    : engine(engineRef)
{
    // Tabs setup
    frequencyTab.setComponentID("displayTab");
    frequencyTab.setClickingTogglesState(false);
    frequencyTab.onClick = [this]
    {
        currentMode = DisplayMode::Frequency;
        spectrumComponent.setVisible(true);
        waveformComponent.setVisible(false);
        resized();
        repaint();
    };
    addAndMakeVisible(frequencyTab);

    waveformTab.setComponentID("displayTab");
    waveformTab.setClickingTogglesState(false);
    waveformTab.onClick = [this]
    {
        currentMode = DisplayMode::Waveform;
        spectrumComponent.setVisible(false);
        waveformComponent.setVisible(true);
        resized();
        repaint();
    };
    addAndMakeVisible(waveformTab);

    addAndMakeVisible(spectrumComponent);
    addAndMakeVisible(waveformComponent);

    // Default to Frequency display
    waveformComponent.setVisible(false);
    spectrumComponent.setVisible(true);

    startTimerHz(30);
}

WeissDisplayComponent::~WeissDisplayComponent()
{
    stopTimer();
}

void WeissDisplayComponent::setOutputLevels(float leftRms, float rightRms)
{
    constexpr float decay = 0.84f;
    outLevelL = std::max(leftRms, outLevelL * decay);
    outLevelR = std::max(rightRms, outLevelR * decay);
}

void WeissDisplayComponent::timerCallback()
{
    repaint();
}

void WeissDisplayComponent::drawVerticalMeter(juce::Graphics& g, const juce::Rectangle<float>& area,
                                             float levelL, float levelR, const juce::String& topLabel, bool hasRedMarker)
{
    const float w = area.getWidth();
    const float h = area.getHeight();

    // Top numeric peak text (e.g. "0.7  0.7" or "0.0")
    g.setFont(juce::FontOptions(8.5f, juce::Font::plain));
    g.setColour(CustomLookAndFeel::textDark);
    g.drawText(topLabel, static_cast<int>(area.getX()), static_cast<int>(area.getY()), static_cast<int>(w), 14, juce::Justification::centred);

    const float meterTop = area.getY() + 16.0f;
    const float meterH = h - 22.0f;
    const float colW = (w - 14.0f) * 0.5f;

    const float col1X = area.getX() + 4.0f;
    const float col2X = col1X + colW + 3.0f;

    // Background columns
    g.setColour(juce::Colour(0xffece9e9));
    g.fillRect(col1X, meterTop, colW, meterH);
    g.fillRect(col2X, meterTop, colW, meterH);

    // Active filled levels (bottom to top)
    const float fillH1 = juce::jlimit(0.0f, meterH, levelL * 2.8f * meterH);
    const float fillH2 = juce::jlimit(0.0f, meterH, levelR * 2.8f * meterH);

    g.setColour(CustomLookAndFeel::textDark);
    g.fillRect(col1X, meterTop + meterH - fillH1, colW, fillH1);
    g.fillRect(col2X, meterTop + meterH - fillH2, colW, fillH2);

    // Segmented tick lines across both columns
    g.setColour(juce::Colour(0xfff7f2f2));
    for (float y = meterTop + 4.0f; y < meterTop + meterH; y += 4.0f)
    {
        g.drawHorizontalLine(static_cast<int>(y), col1X, col1X + colW);
        g.drawHorizontalLine(static_cast<int>(y), col2X, col2X + colW);
    }

    // Outer column borders
    g.setColour(CustomLookAndFeel::displayBorder);
    g.drawRect(col1X, meterTop, colW, meterH, 1.0f);
    g.drawRect(col2X, meterTop, colW, meterH, 1.0f);

    // ARC precision horizontal red threshold/ceiling band
    if (hasRedMarker)
    {
        const float markerY = meterTop + (meterH * 0.72f);
        const float bandH = 8.0f;

        g.setColour(CustomLookAndFeel::arcRed.withAlpha(0.35f));
        g.fillRect(area.getX() + 2.0f, markerY, w - 4.0f, bandH);

        g.setColour(CustomLookAndFeel::arcRed);
        g.drawHorizontalLine(static_cast<int>(markerY), area.getX() + 2.0f, area.getRight() - 2.0f);
        g.drawHorizontalLine(static_cast<int>(markerY + bandH), area.getX() + 2.0f, area.getRight() - 2.0f);
    }
}

void WeissDisplayComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    // Recessed display frame
    g.setColour(juce::Colour(0xfff7f2f2));
    g.fillRoundedRectangle(bounds, 4.0f);

    // Display border with fine 1px dark hairline
    g.setColour(CustomLookAndFeel::displayBorder);
    g.drawRoundedRectangle(bounds, 4.0f, 1.0f);

    // Subtle inner bezel shadow
    g.setColour(juce::Colours::black.withAlpha(0.04f));
    g.drawRoundedRectangle(bounds.reduced(1.0f), 3.0f, 1.0f);

    // Draw Left Input Meters
    const auto leftMeterRect = juce::Rectangle<float>(bounds.getX() + 4.0f, bounds.getY() + 4.0f,
                                                      36.0f, bounds.getHeight() - 30.0f);
    drawVerticalMeter(g, leftMeterRect, outLevelL * 0.9f, outLevelR * 0.9f, "IN", false);

    // Draw Right Output Meters
    const auto rightMeterRect = juce::Rectangle<float>(bounds.getRight() - 40.0f, bounds.getY() + 4.0f,
                                                       36.0f, bounds.getHeight() - 30.0f);
    drawVerticalMeter(g, rightMeterRect, outLevelL, outLevelR, "OUT", true);

    // Bottom Tab Bar separator line
    const float tabY = bounds.getBottom() - 24.0f;
    g.setColour(CustomLookAndFeel::displayBorder);
    g.drawHorizontalLine(static_cast<int>(tabY), bounds.getX(), bounds.getRight());

    // Update tab styling
    const bool isFreq = (currentMode == DisplayMode::Frequency);
    frequencyTab.setToggleState(isFreq, juce::dontSendNotification);
    waveformTab.setToggleState(!isFreq, juce::dontSendNotification);

    frequencyTab.setColour(juce::TextButton::buttonColourId, isFreq ? CustomLookAndFeel::arcCyan : juce::Colours::transparentBlack);
    waveformTab.setColour(juce::TextButton::buttonColourId, !isFreq ? CustomLookAndFeel::arcCyan : juce::Colours::transparentBlack);
}

void WeissDisplayComponent::resized()
{
    const auto bounds = getLocalBounds();
    const int meterW = 44;
    const int tabH = 22;

    // Center Graph Area
    const auto graphArea = juce::Rectangle<int>(
        bounds.getX() + meterW,
        bounds.getY() + 2,
        bounds.getWidth() - (meterW * 2),
        bounds.getHeight() - tabH - 4);

    spectrumComponent.setBounds(graphArea);
    waveformComponent.setBounds(graphArea);

    // Bottom tab bar (centered)
    const int tabY = bounds.getBottom() - tabH - 1;
    const int tabW = 86;

    frequencyTab.setBounds(bounds.getCentreX() - tabW - 4, tabY, tabW, tabH);
    waveformTab.setBounds(bounds.getCentreX() + 4, tabY, tabW, tabH);
}
