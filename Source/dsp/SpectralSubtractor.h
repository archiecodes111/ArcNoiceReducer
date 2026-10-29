#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <cmath>
#include <algorithm>

//==============================================================================
/**
    SpectralSubtractor performs magnitude spectral subtraction, gain computation,
    and scales the complex FFT bins as specified in Sections 4.3, 4.4, and 4.5
    of the Assignment Brief.

    Equations:
    D_x(m,k) = |X(m,k)|
    \hat{D}_s(m,k) = max(D_x(m,k) - \alpha * \bar{D}_v(k), \beta * D_x(m,k))
    G(m,k) = \hat{D}_s(m,k) / (D_x(m,k) + \epsilon)
    Re{Y(m,k)} = G(m,k) * Re{X(m,k)}
    Im{Y(m,k)} = G(m,k) * Im{X(m,k)}
*/
class SpectralSubtractor
{
public:
    SpectralSubtractor() = default;
    ~SpectralSubtractor() = default;

    /** Sets the subtraction factor alpha (\alpha >= 0). */
    void setAlpha(float newAlpha) noexcept
    {
        alpha = std::max(0.0f, newAlpha);
    }

    float getAlpha() const noexcept
    {
        return alpha;
    }

    /**
        Sets an optional spectral floor beta (\beta >= 0, default 0.0).
        A small beta (e.g., 0.02 - 0.05) prevents harsh musical noise artifacts
        as discussed in Section 9 of the brief.
    */
    void setSpectralFloor(float newFloor) noexcept
    {
        spectralFloor = juce::jlimit(0.0f, 1.0f, newFloor);
    }

    float getSpectralFloor() const noexcept
    {
        return spectralFloor;
    }

    /**
        Processes the complex spectrum in-place in the interleaved JUCE FFT buffer.
        fftBuffer contains interleaved [Re, Im] for each bin k:
            Re = fftBuffer[2*k], Im = fftBuffer[2*k + 1]
        noiseProfile contains \bar{D}_v(k) for k in [0, numBins - 1].

        Optional output pointers inputMagOut and outputMagOut can receive
        magnitude spectra for UI visualization.
    */
    void processSpectrum(float* fftBuffer,
                         const float* noiseProfile,
                         int numBins,
                         float* inputMagOut = nullptr,
                         float* outputMagOut = nullptr) const
    {
        constexpr float epsilon = 1e-7f;

        for (int k = 0; k < numBins; ++k)
        {
            const size_t realIdx = static_cast<size_t>(2 * k);
            const size_t imagIdx = realIdx + 1;

            const float re = fftBuffer[realIdx];
            const float im = fftBuffer[imagIdx];

            // 4.3 Magnitude of noisy input: D_x(m,k) = |X(m,k)|
            const float dx = std::sqrt(re * re + im * im);

            if (inputMagOut != nullptr)
                inputMagOut[k] = dx;

            float gain = 1.0f;

            if (noiseProfile != nullptr)
            {
                const float noiseMag = noiseProfile[k];
                const float noiseFloor = spectralFloor * dx;

                // 4.3 Subtraction with clamping at zero (or spectral floor)
                // \hat{D}_s(m,k) = max(D_x(m,k) - \alpha * \bar{D}_v(k), floor)
                const float ds = std::max(dx - (alpha * noiseMag), noiseFloor);

                // 4.4 Gain calculation: G(m,k) = \hat{D}_s(m,k) / (D_x(m,k) + \epsilon)
                gain = ds / (dx + epsilon);

                // Ensure gain is bounded in [0, 1]
                gain = juce::jlimit(0.0f, 1.0f, gain);
            }

            // 4.5 Apply real-valued gain to complex spectrum (preserves phase)
            // Re{Y(m,k)} = G(m,k) * Re{X(m,k)}
            // Im{Y(m,k)} = G(m,k) * Im{X(m,k)}
            fftBuffer[realIdx] = re * gain;
            fftBuffer[imagIdx] = im * gain;

            if (outputMagOut != nullptr)
                outputMagOut[k] = dx * gain;
        }
    }

private:
    float alpha = 1.5f;           // Subtraction factor (default 1.5)
    float spectralFloor = 0.025f; // Locked studio sweet spot: eliminates musical chirps with zero audible tone degradation
};
