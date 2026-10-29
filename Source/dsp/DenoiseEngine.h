#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <vector>
#include <mutex>

#include "AudioFileManager.h"
#include "NoiseProfile.h"
#include "FrameProcessor.h"
#include "SpectralSubtractor.h"
#include "AudioReconstructor.h"

//==============================================================================
/**
    AuditionMode selects which audio stream is routed to the audio output:
    - Denoised: The cleaned, processed audio output
    - Bypass:   The original noisy input audio
    - Delta:    The noise difference signal (input - denoised)
*/
enum class AuditionMode
{
    Denoised = 0,
    Bypass = 1,
    Delta = 2,
    Residual = 2 // Backward-compatibility alias
};

//==============================================================================
/**
    DenoiseEngine orchestrates the entire spectral subtraction pipeline:
    1. Audio file loading/saving
    2. Noise profile estimation (selection or file)
    3. Forward STFT framing and windowing
    4. Magnitude subtraction & complex bin scaling
    5. Inverse STFT, synthesis windowing, overlap-add, and normalisation
    6. Real-time playback transport & 3-way auditioning
*/
class DenoiseEngine
{
public:
    DenoiseEngine();
    ~DenoiseEngine() = default;

    // Parameter configuration
    void setFFTOrder(int order); // e.g. 10 (1024), 11 (2048), 12 (4096)
    int getFFTSize() const noexcept   { return fftSize; }
    int getHopSize() const noexcept   { return hopSize; }

    void setAlpha(float alpha);
    float getAlpha() const noexcept;

    void setSpectralFloor(float floor);
    float getSpectralFloor() const noexcept;

    void setAuditionMode(AuditionMode mode) noexcept { currentAuditionMode.store(mode); }
    AuditionMode getAuditionMode() const noexcept    { return currentAuditionMode.load(); }

    // Core actions
    bool loadInputFile(const juce::File& file, juce::String& error);
    bool loadNoiseFile(const juce::File& file, juce::String& error);
    bool exportProcessedFile(const juce::File& file, int bitDepth, juce::String& error);

    /** Estimates the noise profile \bar{D}_v(k) from a selected sample region of the input audio. */
    bool estimateNoiseFromSelection(int startSample, int numSamples);

    /** Estimates the noise profile \bar{D}_v(k) from the loaded noise file buffer. */
    bool estimateNoiseFromNoiseBuffer();

    /** Executes full offline processing of the input audio with current parameters. */
    void processAudio();

    // Transport and playback streaming
    void startPlayback()  { isPlaying.store(true); }
    void pausePlayback()  { isPlaying.store(false); }
    void stopPlayback()   { isPlaying.store(false); playheadSample.store(0); }
    void seekToSample(int sampleIndex);
    void setLooping(bool shouldLoop) { isLooping.store(shouldLoop); }

    bool getIsPlaying() const noexcept { return isPlaying.load(); }
    bool getIsLooping() const noexcept { return isLooping.load(); }
    int getPlayheadSample() const noexcept { return playheadSample.load(); }

    /** Called from audio thread: fills destination buffer based on active AuditionMode. */
    void getNextAudioBlock(juce::AudioBuffer<float>& outputBuffer, int numSamples);

    // Metadata & Buffers for GUI
    AudioFileManager& getFileManager() noexcept { return fileManager; }
    const NoiseProfile& getNoiseProfile() const noexcept { return noiseProfile; }

    const juce::AudioBuffer<float>& getInputBuffer() const noexcept     { return fileManager.getInputBuffer(); }
    const juce::AudioBuffer<float>& getProcessedBuffer() const noexcept 
    { 
        return processedBuffers[static_cast<size_t>(activeBufferIndex.load(std::memory_order_acquire))]; 
    }
    const juce::AudioBuffer<float>& getResidualBuffer() const noexcept  
    { 
        return residualBuffers[static_cast<size_t>(activeBufferIndex.load(std::memory_order_acquire))]; 
    }

    bool hasProcessedAudio() const noexcept { return isProcessed.load(); }
    double getSampleRate() const noexcept   { return fileManager.getInputSampleRate(); }

    void setOutputTrimDb(float db) noexcept
    {
        outputGainLinear.store(juce::Decibels::decibelsToGain(db));
    }
    float getOutputTrimDb() const noexcept
    {
        return juce::Decibels::gainToDecibels(outputGainLinear.load());
    }

    // Spectrum snapshots for visualizer
    void getSpectrumSnapshot(std::vector<float>& inputMag,
                             std::vector<float>& noiseMag,
                             std::vector<float>& outputMag) const;

private:
    void rebuildFFT(int order);
    void computeInputSpectrum();
    void processAudioInternal();

    AudioFileManager fileManager;
    NoiseProfile noiseProfile;
    FrameProcessor frameProcessor;
    SpectralSubtractor spectralSubtractor;
    AudioReconstructor audioReconstructor;

    std::unique_ptr<juce::dsp::FFT> fft;
    int fftOrder = 11; // Default 2048
    int fftSize = 2048;
    int hopSize = 512;  // 75% overlap for Hann window

    // Double-buffered processed and residual buffers for real-time, glitch-free audio swapping
    std::array<juce::AudioBuffer<float>, 2> processedBuffers;
    std::array<juce::AudioBuffer<float>, 2> residualBuffers;
    std::atomic<int> activeBufferIndex{0};

    // Stored noise selection to preserve noise profile on FFT size changes
    int lastNoiseStartSample = -1;
    int lastNoiseNumSamples = 0;

    std::atomic<bool> isProcessed{false};
    std::atomic<bool> isPlaying{false};
    std::atomic<bool> isLooping{true};
    std::atomic<int> playheadSample{0};
    std::atomic<AuditionMode> currentAuditionMode{AuditionMode::Denoised};
    std::atomic<float> outputGainLinear{1.0f};

    // Spectrum curve display storage
    mutable std::mutex spectrumMutex;
    std::vector<float> displayInputMag;
    std::vector<float> displayNoiseMag;
    std::vector<float> displayOutputMag;

    mutable std::recursive_mutex processMutex;
};
