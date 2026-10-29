#include "AudioFileManager.h"

AudioFileManager::AudioFileManager()
{
    formatManager.registerBasicFormats();
    formatManager.registerFormat(new juce::FlacAudioFormat(), true);
    formatManager.registerFormat(new juce::OggVorbisAudioFormat(), true);
   #if JUCE_WINDOWS
    formatManager.registerFormat(new juce::WindowsMediaAudioFormat(), true);
   #endif
}

bool AudioFileManager::loadInputFile(const juce::File& file, juce::String& errorMessage)
{
    if (!file.existsAsFile())
    {
        errorMessage = "File does not exist: " + file.getFullPathName();
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "Unsupported audio file format: " + file.getFileExtension();
        return false;
    }

    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);

    if (numChannels <= 0 || numSamples <= 0)
    {
        errorMessage = "Audio file contains no playable audio data.";
        return false;
    }

    inputBuffer.setSize(numChannels, numSamples);
    reader->read(&inputBuffer, 0, numSamples, 0, true, true);

    inputSampleRate = reader->sampleRate;
    inputFileName = file.getFileName();
    errorMessage = {};

    return true;
}

bool AudioFileManager::loadNoiseFile(const juce::File& file, juce::String& errorMessage)
{
    if (!file.existsAsFile())
    {
        errorMessage = "Noise file does not exist: " + file.getFullPathName();
        return false;
    }

    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(file));
    if (reader == nullptr)
    {
        errorMessage = "Unsupported audio format for noise profile: " + file.getFileExtension();
        return false;
    }

    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);

    if (numChannels <= 0 || numSamples <= 0)
    {
        errorMessage = "Noise file contains no audio data.";
        return false;
    }

    noiseBuffer.setSize(numChannels, numSamples);
    reader->read(&noiseBuffer, 0, numSamples, 0, true, true);

    noiseSampleRate = reader->sampleRate;
    noiseFileName = file.getFileName();
    errorMessage = {};

    return true;
}

bool AudioFileManager::saveOutputFile(const juce::File& file,
                                      const juce::AudioBuffer<float>& bufferToSave,
                                      double sampleRate,
                                      int bitsPerSample,
                                      juce::String& errorMessage)
{
    if (bufferToSave.getNumSamples() <= 0 || bufferToSave.getNumChannels() <= 0)
    {
        errorMessage = "No processed audio data available to export.";
        return false;
    }

    file.deleteFile();
    std::unique_ptr<juce::OutputStream> outputStream = file.createOutputStream();
    if (outputStream == nullptr)
    {
        errorMessage = "Unable to create output file: " + file.getFullPathName();
        return false;
    }

    juce::WavAudioFormat wavFormat;
    
    // JUCE 8 AudioFormatWriterOptions takes unique_ptr<OutputStream>&
    auto writerOptions = juce::AudioFormatWriterOptions()
                             .withSampleRate(sampleRate)
                             .withNumChannels(static_cast<size_t>(bufferToSave.getNumChannels()))
                             .withBitsPerSample(bitsPerSample);

    std::unique_ptr<juce::AudioFormatWriter> writer(wavFormat.createWriterFor(outputStream, writerOptions));

    if (writer == nullptr)
    {
        errorMessage = "Failed to create WAV writer for: " + file.getFileName();
        return false;
    }

    if (!writer->writeFromAudioSampleBuffer(bufferToSave, 0, bufferToSave.getNumSamples()))
    {
        errorMessage = "Error occurred while writing audio samples to disk.";
        return false;
    }

    errorMessage = {};
    return true;
}
