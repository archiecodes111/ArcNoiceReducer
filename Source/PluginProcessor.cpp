#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout AudioPluginAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Subtraction factor alpha (\alpha \in [0.0, 5.0])
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"alpha", 1},
        "Subtraction Factor (alpha)",
        juce::NormalisableRange<float>(0.0f, 5.0f, 0.05f),
        1.50f,
        juce::AudioParameterFloatAttributes().withLabel("")
    ));

    // Spectral floor beta (\beta \in [0.0, 0.20]) - fixed sweet spot baseline
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"floor", 1},
        "Spectral Floor (beta)",
        juce::NormalisableRange<float>(0.0f, 0.20f, 0.005f),
        0.025f,
        juce::AudioParameterFloatAttributes().withLabel("")
    ));

    // Audition Mode (Denoised, Bypass, Delta)
    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{"auditionMode", 1},
        "Audition Mode",
        juce::StringArray{"Denoised", "Bypass", "Delta"},
        0
    ));

    return { params.begin(), params.end() };
}

juce::AudioProcessor::BusesProperties AudioPluginAudioProcessor::makeBusesProperties()
{
    BusesProperties buses;

    // For VST3/DAW plugins, provide stereo input for DAW track processing.
    // For the Standalone application, do not declare an input bus so the JUCE standalone wrapper
    // never creates or displays the yellow "Audio input is muted to avoid feedback loop" warning bar.
    if (!juce::JUCEApplicationBase::isStandaloneApp())
    {
        buses.addBus(true, "Input", juce::AudioChannelSet::stereo(), true);
    }

    buses.addBus(false, "Output", juce::AudioChannelSet::stereo(), true);
    return buses;
}

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
    : AudioProcessor(makeBusesProperties()),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
    return true;
#else
    return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
    return true;
#else
    return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
    return 1;
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
    return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
    juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
    juce::ignoreUnused(index);
    return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String &newName)
{
    juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused(sampleRate, samplesPerBlock);

    for (auto &level : rmsLevels)
        level.store(0.0f, std::memory_order_relaxed);
}

void AudioPluginAudioProcessor::releaseResources()
{
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout &layouts) const
{
    // Output must be mono or stereo
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input can be disabled (for standalone file playback or muted mic), mono, or stereo
    if (layouts.getMainInputChannelSet() != juce::AudioChannelSet::disabled()
     && layouts.getMainInputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainInputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float> &buffer,
                                             juce::MidiBuffer &midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Check for parameter updates from host DAW automation
    const float alpha = apvts.getRawParameterValue("alpha")->load(std::memory_order_relaxed);
    if (std::abs(alpha - lastAlpha) > 0.001f)
    {
        lastAlpha = alpha;
        denoiseEngine.setAlpha(alpha);
    }

    const float floor = apvts.getRawParameterValue("floor")->load(std::memory_order_relaxed);
    if (std::abs(floor - lastFloor) > 0.0001f)
    {
        lastFloor = floor;
        denoiseEngine.setSpectralFloor(floor);
    }

    const auto* modeParam = dynamic_cast<juce::AudioParameterChoice*>(apvts.getParameter("auditionMode"));
    if (modeParam != nullptr)
    {
        const int hostMode = modeParam->getIndex();
        if (hostMode != lastHostMode && lastHostMode != -1)
        {
            lastHostMode = hostMode;
            denoiseEngine.setAuditionMode(static_cast<AuditionMode>(hostMode));
        }
        else if (lastHostMode == -1)
        {
            lastHostMode = static_cast<int>(denoiseEngine.getAuditionMode());
        }
    }

    if (denoiseEngine.getIsPlaying())
    {
        // Stream audio file playback
        denoiseEngine.getNextAudioBlock(buffer, buffer.getNumSamples());
    }
    else
    {
        // If not playing, keep buffer clean
        buffer.clear();
    }

    // Compute RMS output levels for GUI meter
    for (int channel = 0; channel < std::min(totalNumOutputChannels, 2); ++channel)
    {
        const float rms = buffer.getRMSLevel(channel, 0, buffer.getNumSamples());
        rmsLevels[static_cast<size_t>(channel)].store(rms, std::memory_order_relaxed);
    }
}

//==============================================================================
bool AudioPluginAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor *AudioPluginAudioProcessor::createEditor()
{
    return new AudioPluginAudioProcessorEditor(*this);
}

//==============================================================================
void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock &destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void *data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Entry point for host plugin instantiation
juce::AudioProcessor *JUCE_CALLTYPE createPluginFilter()
{
    return new AudioPluginAudioProcessor();
}
