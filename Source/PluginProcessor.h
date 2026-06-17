#pragma once

#include <JuceHeader.h>
#include "Parameters.h"
#include "MultibandEngine.h"

class MultibandSaturationAudioProcessor : public juce::AudioProcessor
{
public:
    MultibandSaturationAudioProcessor();
    ~MultibandSaturationAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

    MultibandSettings getCurrentSettings() const;

    void setSoloBand (int bandIndex);
    int getSoloBand() const;

    static constexpr int spectrumFftOrder = 11;
    static constexpr int spectrumFftSize = 1 << spectrumFftOrder;
    static constexpr int spectrumNumBins = spectrumFftSize / 2;

    void getSpectrumMagnitudes (float* dest, int numBins) const;
    double getSampleRateForSpectrum() const { return currentSampleRate; }

private:
    void pushSamplesForSpectrum (const juce::AudioBuffer<float>& buffer);

    MultibandEngine engine;
    std::atomic<int> soloBandIndex { -1 };
    double currentSampleRate = 44100.0;

    juce::dsp::FFT spectrumFft { spectrumFftOrder };
    juce::dsp::WindowingFunction<float> spectrumWindow {
        static_cast<size_t> (spectrumFftSize),
        juce::dsp::WindowingFunction<float>::hann,
        true
    };

    std::array<float, spectrumFftSize> spectrumFifo {};
    int spectrumFifoIndex = 0;
    std::array<float, spectrumFftSize * 2> spectrumFftData {};
    std::array<float, spectrumNumBins> spectrumMagnitudes {};
    mutable juce::SpinLock spectrumLock;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandSaturationAudioProcessor)
};
