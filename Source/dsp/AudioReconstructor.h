#pragma once

#include <juce_core/juce_core.h>
#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
/**
    AudioReconstructor performs:
    1. Inverse FFT: z_m[r] = IFFT{Y(m,k)}[r] * (1 / N)
    2. Synthesis windowing: w_s[r]
    3. Overlap-add accumulation across frames
    4. Normalisation by the overlap denominator:
       y[n] = { \sum_m w_s[n - mH] z_m[n - mH] } / { \sum_m w_a[n - mH] w_s[n - mH] }
    as specified in Section 4.6 of the Assignment Brief.
*/
class AudioReconstructor
{
public:
    AudioReconstructor() = default;
    ~AudioReconstructor() = default;

    /**
        Resets and resizes the reconstruction buffers for an output of targetLength samples.
    */
    void reset(int targetLength, int newFFTSize, int newHopSize, const std::vector<float>& analysisWin)
    {
        fftSize = newFFTSize;
        hopSize = newHopSize;
        totalSamples = targetLength;
        analysisWindow = analysisWin;

        // Synthesis window: matching Hann window for optimal COLA smoothness
        synthesisWindow = analysisWindow;

        // Allocate accumulation buffer with padding for end-of-file tails
        const int paddedLength = totalSamples + fftSize + hopSize;
        accumulatedOutput.assign(static_cast<size_t>(paddedLength), 0.0f);
        normalizationDenominator.assign(static_cast<size_t>(paddedLength), 0.0f);
    }

    /**
        Accumulates a synthesized time-domain frame (extracted from IFFT output) into the overlap-add buffer.
        timeFrame must contain at least fftSize samples (the time-domain output of IFFT).
        Note: The 1/N scaling factor of JUCE IFFT is applied here.
    */
    void accumulateFrame(int frameIndex, const float* timeFrame)
    {
        const int frameStart = frameIndex * hopSize;

        for (int r = 0; r < fftSize; ++r)
        {
            const size_t outIdx = static_cast<size_t>(frameStart + r);
            if (outIdx >= accumulatedOutput.size())
                break;

            const float ws = synthesisWindow[static_cast<size_t>(r)];
            const float wa = analysisWindow[static_cast<size_t>(r)];

            // z_m[r]: JUCE performRealOnlyInverseTransform is already scaled to unit gain
            const float zm = timeFrame[r];

            // Multiply by synthesis window and overlap-add: w_s[n - mH] * z_m[n - mH]
            accumulatedOutput[outIdx] += ws * zm;

            // Accumulate window overlap denominator: w_a[n - mH] * w_s[n - mH]
            normalizationDenominator[outIdx] += wa * ws;
        }
    }

    /**
        Finalizes the overlap-add reconstruction by dividing by the window overlap denominator:
        y[n] = accumulatedOutput[n] / normalizationDenominator[n]
        Safe handling of frame boundaries and near-zero denominators.
    */
    void finalizeToBuffer(float* destination, int numSamplesToCopy) const
    {
        constexpr float minDenominator = 0.08f;
        const int count = std::min(numSamplesToCopy, totalSamples);

        for (int n = 0; n < count; ++n)
        {
            const size_t idx = static_cast<size_t>(n);
            const float denom = normalizationDenominator[idx];

            if (denom >= minDenominator)
            {
                destination[n] = accumulatedOutput[idx] / denom;
            }
            else if (denom > 1e-5f)
            {
                // Smooth boundary ramp-in to avoid division by near-zero at frame edges
                destination[n] = (accumulatedOutput[idx] / minDenominator) * (denom / minDenominator);
            }
            else
            {
                destination[n] = 0.0f;
            }

            destination[n] = juce::jlimit(-1.0f, 1.0f, destination[n]);
        }
    }

private:
    int fftSize = 2048;
    int hopSize = 512;
    int totalSamples = 0;

    std::vector<float> analysisWindow;
    std::vector<float> synthesisWindow;
    std::vector<float> accumulatedOutput;
    std::vector<float> normalizationDenominator;
};
