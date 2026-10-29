#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

//==============================================================================
/**
    FrameProcessor segments an input signal into overlapping windowed frames:
    x_w[r] = x[mH + r] * w_a[r]
    for frame index m, hop size H, FFT size N, and analysis window w_a[r].
    (Section 4.1 of Assignment Brief)
*/
class FrameProcessor
{
public:
    enum class WindowType
    {
        Hann,
        Hamming,
        Blackman
    };

    FrameProcessor()
    {
        updateWindow();
    }

    ~FrameProcessor() = default;

    /** Sets the FFT size, hop size, and windowing type. */
    void setParameters(int newFFTSize, int newHopSize, WindowType newWindowType = WindowType::Hann)
    {
        fftSize = newFFTSize;
        hopSize = newHopSize;
        windowType = newWindowType;
        updateWindow();
    }

    int getFFTSize() const noexcept    { return fftSize; }
    int getHopSize() const noexcept    { return hopSize; }
    int getNumBins() const noexcept    { return (fftSize / 2) + 1; }

    const std::vector<float>& getAnalysisWindow() const noexcept
    {
        return analysisWindow;
    }

    /**
        Extracts frame index m from the input channel, applies the analysis window w_a[r],
        and places the windowed time samples into the first N samples of fftBuffer (which must be at least 2*N).
        Zeros the remaining N samples for JUCE FFT real-to-complex transform.
    */
    void prepareFrame(const float* channelData, int totalSamples, int frameIndex, float* fftBuffer) const
    {
        const int frameStart = frameIndex * hopSize;

        for (int r = 0; r < fftSize; ++r)
        {
            const int sampleIdx = frameStart + r;
            float sample = 0.0f;

            if (sampleIdx >= 0 && sampleIdx < totalSamples)
                sample = channelData[sampleIdx];

            fftBuffer[r] = sample * analysisWindow[static_cast<size_t>(r)];
        }

        // Zero out the second half of the 2*N buffer required by JUCE FFT
        std::fill(fftBuffer + fftSize, fftBuffer + (2 * fftSize), 0.0f);
    }

    /**
        Calculates the total number of frames required to cover totalSamples.
    */
    int calculateNumFrames(int totalSamples) const noexcept
    {
        if (totalSamples <= 0 || hopSize <= 0)
            return 0;

        // Ensure we cover the entire signal plus boundary flush
        return static_cast<int>(std::ceil(static_cast<float>(totalSamples) / static_cast<float>(hopSize)));
    }

private:
    void updateWindow()
    {
        analysisWindow.resize(static_cast<size_t>(fftSize));

        const double twoPi = juce::MathConstants<double>::twoPi;
        const double nMinusOne = static_cast<double>(fftSize); // Periodic window for STFT

        for (int r = 0; r < fftSize; ++r)
        {
            double w = 1.0;
            const double phase = (twoPi * r) / nMinusOne;

            switch (windowType)
            {
                case WindowType::Hann:
                    // Standard periodic Hann window: 0.5 * (1 - cos(2*pi*r / N))
                    w = 0.5 * (1.0 - std::cos(phase));
                    break;

                case WindowType::Hamming:
                    w = 0.54 - 0.46 * std::cos(phase);
                    break;

                case WindowType::Blackman:
                    w = 0.42 - 0.5 * std::cos(phase) + 0.08 * std::cos(2.0 * phase);
                    break;
            }

            analysisWindow[static_cast<size_t>(r)] = static_cast<float>(w);
        }
    }

    int fftSize = 2048;
    int hopSize = 512;
    WindowType windowType = WindowType::Hann;
    std::vector<float> analysisWindow;
};
