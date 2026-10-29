#include "SpectrumComponent.h"
#include "CustomLookAndFeel.h"
#include <cmath>

SpectrumComponent::SpectrumComponent()
{
}

void SpectrumComponent::resized()
{
}

void SpectrumComponent::updateSpectra(const std::vector<float>& inMag,
                                      const std::vector<float>& nMag,
                                      const std::vector<float>& outMag,
                                      double sr)
{
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        inputMagnitudes = inMag;
        noiseMagnitudes = nMag;
        outputMagnitudes = outMag;
        currentSampleRate = (sr > 0.0) ? sr : 44100.0;
    }
    repaint();
}

float SpectrumComponent::frequencyToX(float freqHz) const noexcept
{
    const float w = static_cast<float>(getWidth());
    if (w <= 0.0f) return 0.0f;

    const float clampedFreq = juce::jlimit(minFreq, maxFreq, freqHz);
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);
    const float norm = (std::log10(clampedFreq) - logMin) / (logMax - logMin);

    return norm * w;
}

float SpectrumComponent::magnitudeToY(float magLinear) const noexcept
{
    const float h = static_cast<float>(getHeight());
    if (h <= 0.0f) return 0.0f;

    const float db = (magLinear > 1e-6f) ? (20.0f * std::log10(magLinear)) : minDb;
    const float clampedDb = juce::jlimit(minDb, maxDb, db);
    const float norm = (clampedDb - minDb) / (maxDb - minDb);

    return (1.0f - norm) * h;
}

juce::Path SpectrumComponent::buildPathForSpectrum(const std::vector<float>& magnitudes) const
{
    juce::Path p;
    if (magnitudes.size() < 2)
        return p;

    const int numBins = static_cast<int>(magnitudes.size());
    const float binFreqWidth = static_cast<float>(currentSampleRate) / static_cast<float>(2 * (numBins - 1));
    const float h = static_cast<float>(getHeight());

    bool pathStarted = false;

    constexpr int numDisplayPoints = 320;
    const float logMin = std::log10(minFreq);
    const float logMax = std::log10(maxFreq);

    for (int i = 0; i < numDisplayPoints; ++i)
    {
        const float norm = static_cast<float>(i) / static_cast<float>(numDisplayPoints - 1);
        const float freq = std::pow(10.0f, logMin + norm * (logMax - logMin));
        const float binIndexFloat = freq / binFreqWidth;

        const int binLow = juce::jlimit(0, numBins - 1, static_cast<int>(std::floor(binIndexFloat)));
        const int binHigh = juce::jlimit(0, numBins - 1, binLow + 1);
        const float frac = binIndexFloat - static_cast<float>(binLow);

        const float mag = magnitudes[static_cast<size_t>(binLow)] * (1.0f - frac) +
                          magnitudes[static_cast<size_t>(binHigh)] * frac;

        const float x = frequencyToX(freq);
        const float y = juce::jlimit(2.0f, h - 2.0f, magnitudeToY(mag));

        if (!pathStarted)
        {
            p.startNewSubPath(x, y);
            pathStarted = true;
        }
        else
        {
            p.lineTo(x, y);
        }
    }

    return p;
}

void SpectrumComponent::paint(juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const float w = bounds.getWidth();
    const float h = bounds.getHeight();

    // Weiss recessed display background: warm pale rose-cream
    g.setColour(juce::Colour(0xfff7f2f2));
    g.fillRect(bounds);

    // Subtle decibel grid lines (18 dB steps matching Weiss screenshot)
    const float dbLevels[] = { 0.0f, -18.0f, -36.0f, -54.0f, -72.0f };
    g.setFont(juce::FontOptions(9.0f, juce::Font::plain));

    for (float db : dbLevels)
    {
        const float y = magnitudeToY(std::pow(10.0f, db / 20.0f));
        g.setColour(juce::Colour(0xffe8dcdd));
        g.drawHorizontalLine(static_cast<int>(y), 0.0f, w);

        g.setColour(juce::Colour(0xff8a7d80));
        const auto dbStr = juce::String(static_cast<int>(db));
        g.drawText(dbStr, 4, static_cast<int>(y) - 10, 28, 10, juce::Justification::left);
    }

    // Vertical frequency grid lines (delicate rose-grey)
    const float freqs[] = { 50.0f, 100.0f, 250.0f, 500.0f, 1000.0f, 2500.0f, 5000.0f, 10000.0f, 20000.0f };
    for (float f : freqs)
    {
        const float x = frequencyToX(f);
        g.setColour(juce::Colour(0xffe8dcdd));
        g.drawVerticalLine(static_cast<int>(x), 0.0f, h - 14.0f);

        g.setColour(juce::Colour(0xff8a7d80));
        const auto fStr = (f >= 1000.0f) ? (juce::String(f / 1000.0f, (f == 1000.0f || f == 5000.0f || f == 10000.0f || f == 20000.0f) ? 0 : 1) + "k") :
                                           juce::String(static_cast<int>(f));
        g.drawText(fStr, static_cast<int>(x) - 16, static_cast<int>(h) - 13, 32, 11, juce::Justification::centred);
    }

    // Read spectra
    std::vector<float> inMag, nMag, outMag;
    {
        std::lock_guard<std::mutex> lock(dataMutex);
        inMag = inputMagnitudes;
        nMag = noiseMagnitudes;
        outMag = outputMagnitudes;
    }

    if (inMag.empty() && nMag.empty() && outMag.empty())
    {
        g.setColour(juce::Colour(0xff8a7d80));
        g.setFont(juce::FontOptions(12.0f, juce::Font::plain));
        g.drawText("ARC Spectral Visualizer - Ready", bounds, juce::Justification::centred);
        return;
    }

    // 1. Raw Input Spectrum (Muted Slate / Steel)
    if (!inMag.empty())
    {
        auto inputPath = buildPathForSpectrum(inMag);
        g.setColour(juce::Colour(0xff7a8494));
        g.strokePath(inputPath, juce::PathStrokeType(1.0f));
    }

    // 2. Average Noise Profile (Soft Amber fill below curve)
    if (!nMag.empty())
    {
        auto noisePath = buildPathForSpectrum(nMag);

        juce::Path fillPath = noisePath;
        fillPath.lineTo(w, h);
        fillPath.lineTo(0.0f, h);
        fillPath.closeSubPath();

        juce::ColourGradient fillGrad(CustomLookAndFeel::arcAmber.withAlpha(0.15f), 0.0f, 0.0f,
                                      CustomLookAndFeel::arcAmber.withAlpha(0.02f), 0.0f, h, false);
        g.setGradientFill(fillGrad);
        g.fillPath(fillPath);

        g.setColour(CustomLookAndFeel::arcAmber.withAlpha(0.85f));
        g.strokePath(noisePath, juce::PathStrokeType(1.4f));
    }

    // 3. Clean Denoised Output Spectrum (Crisp Deep Slate Line, 1.8px)
    if (!outMag.empty())
    {
        auto outputPath = buildPathForSpectrum(outMag);
        g.setColour(CustomLookAndFeel::arcCyan);
        g.strokePath(outputPath, juce::PathStrokeType(1.8f));
    }

    // Minimalist Legend Badge
    const auto legendBounds = juce::Rectangle<float>(w - 240.0f, 6.0f, 234.0f, 18.0f);
    g.setColour(juce::Colours::white.withAlpha(0.85f));
    g.fillRoundedRectangle(legendBounds, 3.0f);
    g.setColour(juce::Colour(0xffd8c9c9));
    g.drawRoundedRectangle(legendBounds, 3.0f, 1.0f);

    float legX = legendBounds.getX() + 8.0f;
    const float legY = legendBounds.getCentreY();
    g.setFont(juce::FontOptions(10.0f, juce::Font::plain));

    // Input line
    g.setColour(juce::Colour(0xff7a8494));
    g.drawLine(legX, legY, legX + 10.0f, legY, 1.5f);
    legX += 14.0f;
    g.setColour(CustomLookAndFeel::textDark);
    g.drawText("Input", static_cast<int>(legX), static_cast<int>(legY) - 6, 32, 12, juce::Justification::left);
    legX += 38.0f;

    // Noise Profile line
    g.setColour(CustomLookAndFeel::arcAmber);
    g.drawLine(legX, legY, legX + 10.0f, legY, 1.5f);
    legX += 14.0f;
    g.drawText("Noise Profile", static_cast<int>(legX), static_cast<int>(legY) - 6, 68, 12, juce::Justification::left);
    legX += 74.0f;

    // Denoised line
    g.setColour(CustomLookAndFeel::arcCyan);
    g.drawLine(legX, legY, legX + 10.0f, legY, 2.0f);
    legX += 14.0f;
    g.drawText("Denoised", static_cast<int>(legX), static_cast<int>(legY) - 6, 50, 12, juce::Justification::left);
}
