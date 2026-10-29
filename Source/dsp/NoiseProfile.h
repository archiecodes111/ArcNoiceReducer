#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <mutex>

//==============================================================================
/**
    NoiseProfile estimates and holds the average noise magnitude spectrum:
    \bar{D}_v(k) = (1 / M) * \sum_{m=0}^{M-1} |V(m,k)|
    as specified in Section 4.2 of the Assignment Brief.
*/
class NoiseProfile
{
public:
    NoiseProfile() = default;
    ~NoiseProfile() = default;

    /** Prepares the accumulator for an FFT of size N (giving numBins = N/2 + 1). */
    void setFFTSize(int newFFTSize)
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        fftSize = newFFTSize;
        numBins = (fftSize / 2) + 1;
        resetInternal();
    }

    /** Clears any accumulated noise frames. */
    void reset()
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        resetInternal();
    }

    /** Accumulates the magnitude spectrum of a single noise frame:
        D_v(m,k) = |V(m,k)|
    */
    void accumulateFrame(const float* magnitudeBins, int binCount)
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        if (binCount != numBins || binCount <= 0)
            return;

        for (int k = 0; k < numBins; ++k)
            sumMagnitudes[static_cast<size_t>(k)] += magnitudeBins[k];

        ++accumulatedFrames;
    }

    /** Calculates the average noise magnitude profile:
        \bar{D}_v(k) = (1 / M) * \sum_{m=0}^{M-1} |V(m,k)|
    */
    void finalizeAverage()
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        if (accumulatedFrames == 0 || numBins <= 0)
        {
            profileReady = false;
            return;
        }

        const float invM = 1.0f / static_cast<float>(accumulatedFrames);
        for (int k = 0; k < numBins; ++k)
            averageMagnitude[static_cast<size_t>(k)] = sumMagnitudes[static_cast<size_t>(k)] * invM;

        profileReady = true;
    }

    /** Returns whether a valid noise profile has been estimated. */
    bool isReady() const noexcept
    {
        return profileReady;
    }

    /** Returns the number of noise frames M used in the estimation. */
    int getNumFrames() const noexcept
    {
        return accumulatedFrames;
    }

    /** Returns the number of unique FFT bins (N/2 + 1). */
    int getNumBins() const noexcept
    {
        return numBins;
    }

    /** Returns the FFT size associated with this profile. */
    int getFFTSize() const noexcept
    {
        return fftSize;
    }

    /** Returns the average noise magnitude spectrum \bar{D}_v(k). */
    std::vector<float> getAverageProfile() const
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        return averageMagnitude;
    }

    /** Reads the average noise magnitude at a specific bin index k. */
    float getMagnitudeForBin(int k) const noexcept
    {
        if (k >= 0 && k < numBins && profileReady)
            return averageMagnitude[static_cast<size_t>(k)];
        return 0.0f;
    }

    /** Sets the profile directly (e.g. from loaded profile file or channel copy). */
    void setProfileDirectly(const std::vector<float>& profile, int newFFTSize, int frameCount)
    {
        std::lock_guard<std::mutex> lock(profileMutex);
        fftSize = newFFTSize;
        numBins = static_cast<int>(profile.size());
        averageMagnitude = profile;
        accumulatedFrames = frameCount;
        profileReady = (accumulatedFrames > 0 && numBins > 0);
    }

private:
    void resetInternal()
    {
        accumulatedFrames = 0;
        profileReady = false;
        sumMagnitudes.assign(static_cast<size_t>(numBins), 0.0f);
        averageMagnitude.assign(static_cast<size_t>(numBins), 0.0f);
    }

    mutable std::mutex profileMutex;
    int fftSize = 2048;
    int numBins = 1025;
    int accumulatedFrames = 0;
    bool profileReady = false;

    std::vector<float> sumMagnitudes;
    std::vector<float> averageMagnitude;
};
