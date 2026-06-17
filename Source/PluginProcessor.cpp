#include "PluginProcessor.h"
#include "PluginEditor.h"

MultibandSaturationAudioProcessor::MultibandSaturationAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
#endif
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

MultibandSaturationAudioProcessor::~MultibandSaturationAudioProcessor() = default;

const juce::String MultibandSaturationAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool MultibandSaturationAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool MultibandSaturationAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool MultibandSaturationAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double MultibandSaturationAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int MultibandSaturationAudioProcessor::getNumPrograms()
{
    return 1;
}

int MultibandSaturationAudioProcessor::getCurrentProgram()
{
    return 0;
}

void MultibandSaturationAudioProcessor::setCurrentProgram (int) {}

const juce::String MultibandSaturationAudioProcessor::getProgramName (int)
{
    return {};
}

void MultibandSaturationAudioProcessor::changeProgramName (int, const juce::String&) {}

void MultibandSaturationAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    spectrumFifoIndex = 0;
    spectrumFifo.fill (0.0f);
    spectrumMagnitudes.fill (0.0f);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32> (samplesPerBlock);
    spec.numChannels = static_cast<juce::uint32> (juce::jmax (getTotalNumInputChannels(), getTotalNumOutputChannels()));

    engine.prepare (spec);
}

void MultibandSaturationAudioProcessor::releaseResources() {}

#ifndef JucePlugin_PreferredChannelConfigurations
bool MultibandSaturationAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

MultibandSettings MultibandSaturationAudioProcessor::getCurrentSettings() const
{
    MultibandSettings settings;

    settings.inputGainDb = apvts.getRawParameterValue (ParamIDs::inputGain)->load();
    settings.outputGainDb = apvts.getRawParameterValue (ParamIDs::outputGain)->load();
    settings.mix = apvts.getRawParameterValue (ParamIDs::mix)->load();
    settings.lowCrossoverHz = apvts.getRawParameterValue (ParamIDs::lowCrossover)->load();
    settings.highCrossoverHz = apvts.getRawParameterValue (ParamIDs::highCrossover)->load();

    const std::array<std::array<const char*, 6>, 3> bandParamIds {{
        { ParamIDs::lowDrive,  ParamIDs::lowPreGain,  ParamIDs::lowMix,  ParamIDs::lowGain,  ParamIDs::lowType,  ParamIDs::lowBypass },
        { ParamIDs::midDrive,  ParamIDs::midPreGain,  ParamIDs::midMix,  ParamIDs::midGain,  ParamIDs::midType,  ParamIDs::midBypass },
        { ParamIDs::highDrive, ParamIDs::highPreGain, ParamIDs::highMix, ParamIDs::highGain, ParamIDs::highType, ParamIDs::highBypass }
    }};

    for (size_t i = 0; i < bandParamIds.size(); ++i)
    {
        auto& band = settings.bands[i];
        const auto& ids = bandParamIds[i];

        band.drive = apvts.getRawParameterValue (ids[0])->load();
        band.preGainDb = apvts.getRawParameterValue (ids[1])->load();
        band.mix = apvts.getRawParameterValue (ids[2])->load();
        band.gainDb = apvts.getRawParameterValue (ids[3])->load();
        band.type = static_cast<SaturationType> (static_cast<int> (apvts.getRawParameterValue (ids[4])->load()));
        band.bypass = apvts.getRawParameterValue (ids[5])->load() >= 0.5f;
    }

    return settings;
}

void MultibandSaturationAudioProcessor::setSoloBand (int bandIndex)
{
    soloBandIndex.store (juce::jlimit (-1, 2, bandIndex));
}

int MultibandSaturationAudioProcessor::getSoloBand() const
{
    return soloBandIndex.load();
}

void MultibandSaturationAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    const auto totalNumInputChannels = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    if (totalNumInputChannels == 0)
        return;

    pushSamplesForSpectrum (buffer);

    auto settings = getCurrentSettings();
    settings.soloBandIndex = soloBandIndex.load();
    engine.process (buffer, settings);
}

void MultibandSaturationAudioProcessor::pushSamplesForSpectrum (const juce::AudioBuffer<float>& buffer)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples = buffer.getNumSamples();

    if (numChannels == 0 || numSamples == 0)
        return;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float mono = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
            mono += buffer.getSample (channel, sample);

        mono /= static_cast<float> (numChannels);
        spectrumFifo[static_cast<size_t> (spectrumFifoIndex++)] = mono;

        if (spectrumFifoIndex < spectrumFftSize)
            continue;

        spectrumFifoIndex = 0;

        juce::SpinLock::ScopedLockType lock (spectrumLock);
        std::copy (spectrumFifo.begin(), spectrumFifo.end(), spectrumFftData.begin());
        spectrumWindow.multiplyWithWindowingTable (spectrumFftData.data(), spectrumFftSize);
        spectrumFft.performFrequencyOnlyForwardTransform (spectrumFftData.data());

        const auto scale = 1.0f / static_cast<float> (spectrumFftSize);

        for (int bin = 0; bin < spectrumNumBins; ++bin)
            spectrumMagnitudes[static_cast<size_t> (bin)] = spectrumFftData[static_cast<size_t> (bin)] * scale;
    }
}

void MultibandSaturationAudioProcessor::getSpectrumMagnitudes (float* dest, int numBins) const
{
    const auto binsToCopy = juce::jmin (numBins, spectrumNumBins);
    juce::SpinLock::ScopedLockType lock (spectrumLock);
    std::memcpy (dest, spectrumMagnitudes.data(), static_cast<size_t> (binsToCopy) * sizeof (float));
}

bool MultibandSaturationAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* MultibandSaturationAudioProcessor::createEditor()
{
    return new MultibandSaturationAudioProcessorEditor (*this);
}

void MultibandSaturationAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState().createXml())
        copyXmlToBinary (*state, destData);
}

void MultibandSaturationAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new MultibandSaturationAudioProcessor();
}
