#pragma once

#include <juce_core/juce_core.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_audio_basics/juce_audio_basics.h>

//==============================================================================
/**
    AudioFileManager manages:
    1. Loading input audio files (WAV, MP3, AIFF, FLAC, OGG)
    2. Loading noise-only audio files
    3. Saving/exporting processed audio files
    (Section 6 & 3 of Assignment Brief)
*/
class AudioFileManager
{
public:
    AudioFileManager();
    ~AudioFileManager() = default;

    /** Loads an audio file from disk into the primary input buffer. */
    bool loadInputFile(const juce::File& file, juce::String& errorMessage);

    /** Loads a dedicated noise-only audio file from disk. */
    bool loadNoiseFile(const juce::File& file, juce::String& errorMessage);

    /** Saves an audio buffer to a WAV file on disk. */
    bool saveOutputFile(const juce::File& file,
                        const juce::AudioBuffer<float>& bufferToSave,
                        double sampleRate,
                        int bitsPerSample,
                        juce::String& errorMessage);

    // Buffers and audio metadata
    const juce::AudioBuffer<float>& getInputBuffer() const noexcept { return inputBuffer; }
    juce::AudioBuffer<float>& getInputBuffer() noexcept             { return inputBuffer; }

    const juce::AudioBuffer<float>& getNoiseBuffer() const noexcept { return noiseBuffer; }
    juce::AudioBuffer<float>& getNoiseBuffer() noexcept             { return noiseBuffer; }

    bool hasInputAudio() const noexcept  { return inputBuffer.getNumSamples() > 0; }
    bool hasNoiseAudio() const noexcept  { return noiseBuffer.getNumSamples() > 0; }

    double getInputSampleRate() const noexcept { return inputSampleRate; }
    double getNoiseSampleRate() const noexcept { return noiseSampleRate; }

    juce::String getInputFileName() const noexcept { return inputFileName; }
    juce::String getNoiseFileName() const noexcept { return noiseFileName; }

    double getInputDurationSeconds() const noexcept
    {
        return (inputSampleRate > 0.0) ? (static_cast<double>(inputBuffer.getNumSamples()) / inputSampleRate) : 0.0;
    }

private:
    juce::AudioFormatManager formatManager;

    juce::AudioBuffer<float> inputBuffer;
    double inputSampleRate = 44100.0;
    juce::String inputFileName;

    juce::AudioBuffer<float> noiseBuffer;
    double noiseSampleRate = 44100.0;
    juce::String noiseFileName;
};
