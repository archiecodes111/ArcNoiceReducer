#include "DenoiseEngine.h"
#include <cmath>

DenoiseEngine::DenoiseEngine()
{
    rebuildFFT(fftOrder);
}

void DenoiseEngine::setFFTOrder(int order)
{
    const int clampedOrder = juce::jlimit(9, 13, order); // 512 to 8192
    if (clampedOrder != fftOrder)
    {
        std::lock_guard<std::recursive_mutex> lock(processMutex);
        fftOrder = clampedOrder;
        rebuildFFT(fftOrder);

        if (fileManager.hasInputAudio())
        {
            // 1. Maintain input spectrum curve for the new FFT bin resolution
            computeInputSpectrum();

            // 2. Re-estimate noise profile if previously selected
            if (lastNoiseNumSamples >= fftSize && lastNoiseStartSample >= 0)
            {
                estimateNoiseFromSelection(lastNoiseStartSample, lastNoiseNumSamples);
            }

            // 3. Re-process audio if previously processed, updating active buffer and cyan spectrum in real-time
            if (isProcessed.load(std::memory_order_acquire))
            {
                processAudioInternal();
            }
        }
    }
}

void DenoiseEngine::rebuildFFT(int order)
{
    fftOrder = order;
    fftSize = 1 << fftOrder;
    hopSize = fftSize / 4; // 75% overlap for smooth Hann reconstruction

    fft = std::make_unique<juce::dsp::FFT>(fftOrder);
    frameProcessor.setParameters(fftSize, hopSize, FrameProcessor::WindowType::Hann);
    noiseProfile.setFFTSize(fftSize);
}

void DenoiseEngine::computeInputSpectrum()
{
    const auto& inputBuf = fileManager.getInputBuffer();
    const int totalSamples = inputBuf.getNumSamples();
    const int numChannels = inputBuf.getNumChannels();
    const int numBins = (fftSize / 2) + 1;

    std::vector<float> avgInputMag(static_cast<size_t>(numBins), 0.0f);
    std::vector<float> fftWorkspace(static_cast<size_t>(2 * fftSize), 0.0f);

    const int totalFrames = frameProcessor.calculateNumFrames(totalSamples);
    const int framesToAnalyze = std::min(totalFrames, 64);
    if (framesToAnalyze > 0 && numChannels > 0)
    {
        const float* readPtr = inputBuf.getReadPointer(0);
        for (int frameIdx = 0; frameIdx < framesToAnalyze; ++frameIdx)
        {
            frameProcessor.prepareFrame(readPtr, totalSamples, frameIdx, fftWorkspace.data());
            fft->performRealOnlyForwardTransform(fftWorkspace.data(), false);

            for (int k = 0; k < numBins; ++k)
            {
                const float re = fftWorkspace[static_cast<size_t>(2 * k)];
                const float im = fftWorkspace[static_cast<size_t>(2 * k + 1)];
                avgInputMag[static_cast<size_t>(k)] += std::sqrt(re * re + im * im);
            }
        }

        const float invFrames = 1.0f / static_cast<float>(framesToAnalyze);
        for (int k = 0; k < numBins; ++k)
            avgInputMag[static_cast<size_t>(k)] *= invFrames;
    }

    {
        std::lock_guard<std::mutex> specLock(spectrumMutex);
        displayInputMag = std::move(avgInputMag);
    }
}

void DenoiseEngine::setAlpha(float alpha)
{
    spectralSubtractor.setAlpha(alpha);
}

float DenoiseEngine::getAlpha() const noexcept
{
    return spectralSubtractor.getAlpha();
}

void DenoiseEngine::setSpectralFloor(float floor)
{
    spectralSubtractor.setSpectralFloor(floor);
}

float DenoiseEngine::getSpectralFloor() const noexcept
{
    return spectralSubtractor.getSpectralFloor();
}

bool DenoiseEngine::loadInputFile(const juce::File& file, juce::String& error)
{
    stopPlayback();

    if (!fileManager.loadInputFile(file, error))
        return false;

    // Reset processing state and noise profile
    isProcessed.store(false, std::memory_order_release);
    playheadSample.store(0);
    noiseProfile.reset();
    lastNoiseStartSample = -1;
    lastNoiseNumSamples = 0;

    const auto& inputBuf = fileManager.getInputBuffer();
    const int totalSamples = inputBuf.getNumSamples();
    const int numChannels = inputBuf.getNumChannels();

    for (int i = 0; i < 2; ++i)
    {
        processedBuffers[static_cast<size_t>(i)].setSize(numChannels, totalSamples);
        processedBuffers[static_cast<size_t>(i)].makeCopyOf(inputBuf);

        residualBuffers[static_cast<size_t>(i)].setSize(numChannels, totalSamples);
        residualBuffers[static_cast<size_t>(i)].clear();
    }
    activeBufferIndex.store(0, std::memory_order_release);

    computeInputSpectrum();

    // Keep noise profile and output curve clean until user actions
    {
        std::lock_guard<std::mutex> specLock(spectrumMutex);
        displayNoiseMag.clear();
        displayOutputMag.clear();
    }

    return true;
}

bool DenoiseEngine::loadNoiseFile(const juce::File& file, juce::String& error)
{
    if (!fileManager.loadNoiseFile(file, error))
        return false;

    return estimateNoiseFromNoiseBuffer();
}

bool DenoiseEngine::exportProcessedFile(const juce::File& file, int bitDepth, juce::String& error)
{
    if (!isProcessed.load(std::memory_order_acquire) || getProcessedBuffer().getNumSamples() <= 0)
    {
        error = "Please process the audio before exporting.";
        return false;
    }

    const int activeIdx = activeBufferIndex.load(std::memory_order_acquire);
    return fileManager.saveOutputFile(file, processedBuffers[static_cast<size_t>(activeIdx)], fileManager.getInputSampleRate(), bitDepth, error);
}

bool DenoiseEngine::estimateNoiseFromSelection(int startSample, int numSamples)
{
    std::lock_guard<std::recursive_mutex> lock(processMutex);

    const auto& inputBuf = fileManager.getInputBuffer();
    if (inputBuf.getNumSamples() <= 0 || numSamples < fftSize)
        return false;

    const int safeStart = juce::jlimit(0, inputBuf.getNumSamples() - 1, startSample);
    const int safeCount = std::min(numSamples, inputBuf.getNumSamples() - safeStart);

    // Save selection bounds to preserve noise profile on FFT size changes
    lastNoiseStartSample = safeStart;
    lastNoiseNumSamples = safeCount;

    noiseProfile.reset();
    const int numBins = (fftSize / 2) + 1;

    std::vector<float> fftWorkspace(static_cast<size_t>(2 * fftSize), 0.0f);
    std::vector<float> frameMag(static_cast<size_t>(numBins), 0.0f);

    const int numFrames = (safeCount - fftSize) / hopSize + 1;
    const int numChannels = inputBuf.getNumChannels();

    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx)
    {
        const int frameStart = safeStart + (frameIdx * hopSize);

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = inputBuf.getReadPointer(ch);

            // Analysis windowing
            for (int r = 0; r < fftSize; ++r)
            {
                const int s = frameStart + r;
                const float sample = (s < inputBuf.getNumSamples()) ? channelData[s] : 0.0f;
                fftWorkspace[static_cast<size_t>(r)] = sample * frameProcessor.getAnalysisWindow()[static_cast<size_t>(r)];
            }
            std::fill(fftWorkspace.begin() + fftSize, fftWorkspace.end(), 0.0f);

            // Forward FFT
            fft->performRealOnlyForwardTransform(fftWorkspace.data(), false);

            // Compute magnitude: D_v(m,k) = |V(m,k)|
            for (int k = 0; k < numBins; ++k)
            {
                const float re = fftWorkspace[static_cast<size_t>(2 * k)];
                const float im = fftWorkspace[static_cast<size_t>(2 * k + 1)];
                frameMag[static_cast<size_t>(k)] = std::sqrt(re * re + im * im);
            }

            noiseProfile.accumulateFrame(frameMag.data(), numBins);
        }
    }

    noiseProfile.finalizeAverage();

    // Copy to display curve (shows amber curve on spectrum analyzer)
    {
        std::lock_guard<std::mutex> specLock(spectrumMutex);
        displayNoiseMag = noiseProfile.getAverageProfile();
    }

    // Note: Do not auto-process here; user clicks '2. PROCESS AUDIO' explicitly
    return true;
}

bool DenoiseEngine::estimateNoiseFromNoiseBuffer()
{
    std::lock_guard<std::recursive_mutex> lock(processMutex);

    const auto& noiseBuf = fileManager.getNoiseBuffer();
    if (noiseBuf.getNumSamples() <= 0 || noiseBuf.getNumSamples() < fftSize)
        return false;

    noiseProfile.reset();
    const int numBins = (fftSize / 2) + 1;

    std::vector<float> fftWorkspace(static_cast<size_t>(2 * fftSize), 0.0f);
    std::vector<float> frameMag(static_cast<size_t>(numBins), 0.0f);

    const int numFrames = (noiseBuf.getNumSamples() - fftSize) / hopSize + 1;
    const int numChannels = noiseBuf.getNumChannels();

    for (int frameIdx = 0; frameIdx < numFrames; ++frameIdx)
    {
        const int frameStart = frameIdx * hopSize;

        for (int ch = 0; ch < numChannels; ++ch)
        {
            const float* channelData = noiseBuf.getReadPointer(ch);

            for (int r = 0; r < fftSize; ++r)
            {
                const int s = frameStart + r;
                const float sample = (s < noiseBuf.getNumSamples()) ? channelData[s] : 0.0f;
                fftWorkspace[static_cast<size_t>(r)] = sample * frameProcessor.getAnalysisWindow()[static_cast<size_t>(r)];
            }
            std::fill(fftWorkspace.begin() + fftSize, fftWorkspace.end(), 0.0f);

            fft->performRealOnlyForwardTransform(fftWorkspace.data(), false);

            for (int k = 0; k < numBins; ++k)
            {
                const float re = fftWorkspace[static_cast<size_t>(2 * k)];
                const float im = fftWorkspace[static_cast<size_t>(2 * k + 1)];
                frameMag[static_cast<size_t>(k)] = std::sqrt(re * re + im * im);
            }

            noiseProfile.accumulateFrame(frameMag.data(), numBins);
        }
    }

    noiseProfile.finalizeAverage();

    {
        std::lock_guard<std::mutex> specLock(spectrumMutex);
        displayNoiseMag = noiseProfile.getAverageProfile();
    }

    // Noise profile is ready; user can click '2. PROCESS AUDIO' to apply
    return true;
}

void DenoiseEngine::processAudio()
{
    std::lock_guard<std::recursive_mutex> lock(processMutex);
    processAudioInternal();
}

void DenoiseEngine::processAudioInternal()
{
    const auto& inputBuf = fileManager.getInputBuffer();
    const int totalSamples = inputBuf.getNumSamples();
    const int numChannels = inputBuf.getNumChannels();

    if (totalSamples <= 0 || numChannels <= 0)
        return;

    const int numBins = (fftSize / 2) + 1;
    const auto averageNoise = noiseProfile.getAverageProfile();
    const float* noisePtr = noiseProfile.isReady() ? averageNoise.data() : nullptr;

    const int totalFrames = frameProcessor.calculateNumFrames(totalSamples);

    juce::AudioBuffer<float> tempProcessed(numChannels, totalSamples);
    juce::AudioBuffer<float> tempResidual(numChannels, totalSamples);

    std::vector<float> fftWorkspace(static_cast<size_t>(2 * fftSize), 0.0f);
    std::vector<float> avgInputMag(static_cast<size_t>(numBins), 0.0f);
    std::vector<float> avgOutputMag(static_cast<size_t>(numBins), 0.0f);
    std::vector<float> frameInputMag(static_cast<size_t>(numBins), 0.0f);
    std::vector<float> frameOutputMag(static_cast<size_t>(numBins), 0.0f);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* readPtr = inputBuf.getReadPointer(ch);
        float* writePtr = tempProcessed.getWritePointer(ch);

        audioReconstructor.reset(totalSamples, fftSize, hopSize, frameProcessor.getAnalysisWindow());

        for (int frameIdx = 0; frameIdx < totalFrames; ++frameIdx)
        {
            // 4.1 Windowed overlapping frame
            frameProcessor.prepareFrame(readPtr, totalSamples, frameIdx, fftWorkspace.data());

            // Forward FFT
            fft->performRealOnlyForwardTransform(fftWorkspace.data(), false);

            // 4.3 - 4.5 Magnitude spectral subtraction and gain scaling
            spectralSubtractor.processSpectrum(
                fftWorkspace.data(),
                noisePtr,
                numBins,
                (ch == 0) ? frameInputMag.data() : nullptr,
                (ch == 0) ? frameOutputMag.data() : nullptr);

            if (ch == 0)
            {
                for (int k = 0; k < numBins; ++k)
                {
                    avgInputMag[static_cast<size_t>(k)] += frameInputMag[static_cast<size_t>(k)];
                    avgOutputMag[static_cast<size_t>(k)] += frameOutputMag[static_cast<size_t>(k)];
                }
            }

            // 4.6 Inverse FFT
            fft->performRealOnlyInverseTransform(fftWorkspace.data());

            // Overlap-add accumulation with synthesis windowing and 1/N scaling
            audioReconstructor.accumulateFrame(frameIdx, fftWorkspace.data());
        }

        // Finalise with window normalisation
        audioReconstructor.finalizeToBuffer(writePtr, totalSamples);
    }

    // Compute residual difference buffer: residual[n] = input[n] - processed[n]
    for (int ch = 0; ch < numChannels; ++ch)
    {
        const float* inData = inputBuf.getReadPointer(ch);
        const float* procData = tempProcessed.getReadPointer(ch);
        float* resData = tempResidual.getWritePointer(ch);

        for (int n = 0; n < totalSamples; ++n)
            resData[n] = inData[n] - procData[n];
    }

    // Normalize display spectra
    if (totalFrames > 0)
    {
        const float invFrames = 1.0f / static_cast<float>(totalFrames);
        for (int k = 0; k < numBins; ++k)
        {
            avgInputMag[static_cast<size_t>(k)] *= invFrames;
            avgOutputMag[static_cast<size_t>(k)] *= invFrames;
        }

        std::lock_guard<std::mutex> specLock(spectrumMutex);
        displayInputMag = std::move(avgInputMag);
        displayOutputMag = std::move(avgOutputMag);
    }

    // Double-buffered swap: populate the inactive buffer index, then swap atomically
    const int currentActive = activeBufferIndex.load(std::memory_order_relaxed);
    const int writeIdx = 1 - currentActive;

    processedBuffers[static_cast<size_t>(writeIdx)] = std::move(tempProcessed);
    residualBuffers[static_cast<size_t>(writeIdx)] = std::move(tempResidual);

    activeBufferIndex.store(writeIdx, std::memory_order_release);
    isProcessed.store(true, std::memory_order_release);
}

void DenoiseEngine::seekToSample(int sampleIndex)
{
    const int total = fileManager.getInputBuffer().getNumSamples();
    if (total > 0)
        playheadSample.store(juce::jlimit(0, total - 1, sampleIndex));
    else
        playheadSample.store(0);
}

void DenoiseEngine::getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer, int numSamples)
{
    const auto& inputBuf = fileManager.getInputBuffer();
    const int totalSamples = inputBuf.getNumSamples();

    if (!isPlaying.load(std::memory_order_relaxed) || totalSamples <= 0 || numSamples <= 0)
    {
        outputBuffer.clear();
        return;
    }

    const int activeIdx = activeBufferIndex.load(std::memory_order_acquire);
    const auto mode = currentAuditionMode.load(std::memory_order_relaxed);
    const juce::AudioBuffer<float>* sourceBuffer = nullptr;

    if (mode == AuditionMode::Bypass)
    {
        sourceBuffer = &inputBuf;
    }
    else if (mode == AuditionMode::Delta || mode == AuditionMode::Residual)
    {
        sourceBuffer = isProcessed.load(std::memory_order_acquire) ? &residualBuffers[static_cast<size_t>(activeIdx)] : &inputBuf;
    }
    else // AuditionMode::Denoised
    {
        sourceBuffer = isProcessed.load(std::memory_order_acquire) ? &processedBuffers[static_cast<size_t>(activeIdx)] : &inputBuf;
    }

    const int srcChannels = sourceBuffer->getNumChannels();
    const int srcSamples = sourceBuffer->getNumSamples();
    const int outChannels = outputBuffer.getNumChannels();

    if (srcChannels <= 0 || srcSamples <= 0 || outChannels <= 0)
    {
        outputBuffer.clear();
        return;
    }

    int currentPos = playheadSample.load();

    for (int sampleIdx = 0; sampleIdx < numSamples; ++sampleIdx)
    {
        if (currentPos >= srcSamples)
        {
            if (isLooping.load())
            {
                currentPos = 0;
            }
            else
            {
                isPlaying.store(false);
                for (int ch = 0; ch < outChannels; ++ch)
                    outputBuffer.setSample(ch, sampleIdx, 0.0f);
                continue;
            }
        }

        const float gain = outputGainLinear.load();
        for (int ch = 0; ch < outChannels; ++ch)
        {
            const int srcCh = juce::jlimit(0, srcChannels - 1, ch);
            const int readPos = juce::jlimit(0, srcSamples - 1, currentPos);
            const float val = sourceBuffer->getSample(srcCh, readPos) * gain;
            outputBuffer.setSample(ch, sampleIdx, val);
        }

        ++currentPos;
    }

    playheadSample.store(currentPos);
}

void DenoiseEngine::getSpectrumSnapshot(std::vector<float>& inputMag,
                                       std::vector<float>& noiseMag,
                                       std::vector<float>& outputMag) const
{
    std::lock_guard<std::mutex> lock(spectrumMutex);
    inputMag = displayInputMag;
    noiseMag = displayNoiseMag;
    outputMag = displayOutputMag;
}
