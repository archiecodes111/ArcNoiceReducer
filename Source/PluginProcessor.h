#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <atomic>

#include "dsp/DenoiseEngine.h"

//==============================================================================
class AudioPluginAudioProcessor final : public juce::AudioProcessor
{
public:
    //==============================================================================
    AudioPluginAudioProcessor();
    ~AudioPluginAudioProcessor() override;

    //==============================================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================================
    // APVTS Parameter Management
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Lock-free RMS output level reading for GUI meters
    float getRmsLevel(int channel) const noexcept
    {
        jassert(channel >= 0 && channel < 2);
        return rmsLevels[static_cast<size_t>(channel)].load(std::memory_order_relaxed);
    }

    DenoiseEngine& getDenoiseEngine() noexcept { return denoiseEngine; }

private:
    static BusesProperties makeBusesProperties();

    //==============================================================================
    DenoiseEngine denoiseEngine;
    std::array<std::atomic<float>, 2> rmsLevels{{0.0f, 0.0f}};
    float lastAlpha = -1.0f;
    float lastFloor = -1.0f;
    int lastHostMode = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};

// Convenience alias matching the CMake project name
using NoiseCleanerAudioProcessor = AudioPluginAudioProcessor;
