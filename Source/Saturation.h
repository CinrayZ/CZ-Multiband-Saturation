#pragma once

#include <JuceHeader.h>
#include <cmath>

enum class SaturationType
{
    soft = 0,
    hard,
    tube,
    tape
};

struct Saturation
{
    static float driveToGain (float drivePercent) noexcept
    {
        const auto normalized = juce::jlimit (0.0f, 100.0f, drivePercent) * 0.01f;
        return 1.0f + normalized * normalized * 19.0f;
    }

    static float processSample (float sample, SaturationType type) noexcept
    {
        switch (type)
        {
            case SaturationType::soft:
                return std::tanh (sample);

            case SaturationType::hard:
                return juce::jlimit (-1.0f, 1.0f, sample);

            case SaturationType::tube:
            {
                const auto positive = sample >= 0.0f;
                const auto shaped = std::tanh (std::abs (sample) * 1.15f);
                return positive ? shaped : shaped * 0.88f;
            }

            case SaturationType::tape:
            {
                const auto x = sample;
                const auto cubed = x * x * x;
                return std::tanh (x + 0.12f * cubed);
            }

            default:
                return sample;
        }
    }

    static float apply (float drySample, float wetSample, float mixPercent) noexcept
    {
        const auto mix = juce::jlimit (0.0f, 100.0f, mixPercent) * 0.01f;
        return drySample + (wetSample - drySample) * mix;
    }
};
