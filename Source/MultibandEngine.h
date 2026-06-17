#pragma once

#include "Saturation.h"
#include <array>

struct BandSettings
{
    float drive = 0.0f;
    float preGainDb = 0.0f;
    float mix = 100.0f;
    float gainDb = 0.0f;
    SaturationType type = SaturationType::soft;
    bool bypass = false;
};

struct MultibandSettings
{
    float inputGainDb = 0.0f;
    float outputGainDb = 0.0f;
    float mix = 100.0f;
    float lowCrossoverHz = 120.0f;
    float highCrossoverHz = 4000.0f;
    int soloBandIndex = -1;
    std::array<BandSettings, 3> bands {};
};

class MultibandEngine
{
public:
    void prepare (const juce::dsp::ProcessSpec& spec)
    {
        sampleRate = spec.sampleRate;
        numChannels = static_cast<int> (spec.numChannels);

        lowMidSplit.prepare (spec);
        midHighSplit.prepare (spec);

        lowMidSplit.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);
        midHighSplit.setType (juce::dsp::LinkwitzRileyFilterType::lowpass);

        lowBandBuffer.setSize (numChannels, static_cast<int> (spec.maximumBlockSize));
        midBandBuffer.setSize (numChannels, static_cast<int> (spec.maximumBlockSize));
        highBandBuffer.setSize (numChannels, static_cast<int> (spec.maximumBlockSize));
        dryBuffer.setSize (numChannels, static_cast<int> (spec.maximumBlockSize));

        reset();
        updateCrossovers (120.0f, 4000.0f);
    }

    void reset()
    {
        lowMidSplit.reset();
        midHighSplit.reset();
    }

    void updateCrossovers (float lowHz, float highHz)
    {
        const auto minGap = 100.0f;
        const auto nyquist = static_cast<float> (sampleRate * 0.49);
        const auto low = juce::jlimit (20.0f, nyquist - minGap, lowHz);
        const auto high = juce::jlimit (low + minGap, nyquist, highHz);

        lowMidSplit.setCutoffFrequency (low);
        midHighSplit.setCutoffFrequency (high);
    }

    void process (juce::AudioBuffer<float>& buffer, const MultibandSettings& settings)
    {
        const auto numSamples = buffer.getNumSamples();
        if (numSamples == 0 || numChannels == 0)
            return;

        updateCrossovers (settings.lowCrossoverHz, settings.highCrossoverHz);

        const auto inputGain = juce::Decibels::decibelsToGain (settings.inputGainDb);
        const auto outputGain = juce::Decibels::decibelsToGain (settings.outputGainDb);
        const auto globalMix = juce::jlimit (0.0f, 100.0f, settings.mix) * 0.01f;

        dryBuffer.makeCopyOf (buffer, true);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* input = buffer.getWritePointer (channel);
            auto* lowOut = lowBandBuffer.getWritePointer (channel);
            auto* midOut = midBandBuffer.getWritePointer (channel);
            auto* highOut = highBandBuffer.getWritePointer (channel);

            for (int i = 0; i < numSamples; ++i)
            {
                const auto sample = input[i] * inputGain;

                float lowSample = 0.0f;
                float midHighSample = 0.0f;
                lowMidSplit.processSample (channel, sample, lowSample, midHighSample);

                float midSample = 0.0f;
                float highSample = 0.0f;
                midHighSplit.processSample (channel, midHighSample, midSample, highSample);

                lowOut[i] = lowSample;
                midOut[i] = midSample;
                highOut[i] = highSample;
            }
        }

        const std::array<const BandSettings*, 3> bandSettings {
            &settings.bands[0],
            &settings.bands[1],
            &settings.bands[2]
        };

        juce::AudioBuffer<float>* bandBuffers[] {
            &lowBandBuffer,
            &midBandBuffer,
            &highBandBuffer
        };

        for (size_t bandIndex = 0; bandIndex < bandSettings.size(); ++bandIndex)
        {
            const auto& band = *bandSettings[bandIndex];
            auto& bandBuffer = *bandBuffers[bandIndex];
            const auto driveGain = Saturation::driveToGain (band.drive);
            const auto preGain = juce::Decibels::decibelsToGain (band.preGainDb);
            const auto postGain = juce::Decibels::decibelsToGain (band.gainDb);
            const auto type = band.type;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                auto* data = bandBuffer.getWritePointer (channel);

                for (int i = 0; i < numSamples; ++i)
                {
                    const auto dry = data[i];

                    if (band.bypass)
                    {
                        data[i] = dry * preGain * postGain;
                        continue;
                    }

                    const auto driven = dry * preGain * driveGain;
                    const auto saturated = Saturation::processSample (driven, type);
                    const auto compensated = saturated / juce::jmax (1.0f, driveGain * 0.65f);
                    const auto wet = compensated * postGain;
                    data[i] = Saturation::apply (dry, wet, band.mix);
                }
            }
        }

        const auto isSoloing = settings.soloBandIndex >= 0 && settings.soloBandIndex < 3;

        for (int channel = 0; channel < numChannels; ++channel)
        {
            auto* output = buffer.getWritePointer (channel);
            const auto* dry = dryBuffer.getReadPointer (channel);
            const auto* low = lowBandBuffer.getReadPointer (channel);
            const auto* mid = midBandBuffer.getReadPointer (channel);
            const auto* high = highBandBuffer.getReadPointer (channel);
            const float* const soloBand = bandBuffers[static_cast<size_t> (settings.soloBandIndex)]->getReadPointer (channel);

            for (int i = 0; i < numSamples; ++i)
            {
                if (isSoloing)
                {
                    output[i] = soloBand[i] * outputGain;
                }
                else
                {
                    const auto wet = (low[i] + mid[i] + high[i]) * outputGain;
                    output[i] = dry[i] + (wet - dry[i]) * globalMix;
                }
            }
        }
    }

private:
    double sampleRate = 44100.0;
    int numChannels = 2;

    juce::dsp::LinkwitzRileyFilter<float> lowMidSplit;
    juce::dsp::LinkwitzRileyFilter<float> midHighSplit;

    juce::AudioBuffer<float> lowBandBuffer;
    juce::AudioBuffer<float> midBandBuffer;
    juce::AudioBuffer<float> highBandBuffer;
    juce::AudioBuffer<float> dryBuffer;
};
