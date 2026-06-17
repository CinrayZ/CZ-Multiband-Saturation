#pragma once

#include <JuceHeader.h>

namespace ParamIDs
{
    inline constexpr auto inputGain   = "inputGain";
    inline constexpr auto outputGain  = "outputGain";
    inline constexpr auto mix         = "mix";
    inline constexpr auto lowCrossover  = "lowCrossover";
    inline constexpr auto highCrossover = "highCrossover";

    inline constexpr auto lowDrive   = "lowDrive";
    inline constexpr auto lowPreGain = "lowPreGain";
    inline constexpr auto lowMix     = "lowMix";
    inline constexpr auto lowGain    = "lowGain";
    inline constexpr auto lowType   = "lowType";
    inline constexpr auto lowBypass = "lowBypass";

    inline constexpr auto midDrive   = "midDrive";
    inline constexpr auto midPreGain = "midPreGain";
    inline constexpr auto midMix     = "midMix";
    inline constexpr auto midGain    = "midGain";
    inline constexpr auto midType   = "midType";
    inline constexpr auto midBypass = "midBypass";

    inline constexpr auto highDrive   = "highDrive";
    inline constexpr auto highPreGain = "highPreGain";
    inline constexpr auto highMix     = "highMix";
    inline constexpr auto highGain    = "highGain";
    inline constexpr auto highType   = "highType";
    inline constexpr auto highBypass = "highBypass";
}

namespace ParamChoices
{
    inline const juce::StringArray saturationTypes { "Soft", "Hard", "Tube", "Tape" };
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::inputGain, 1 },
        "Input",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::outputGain, 1 },
        "Output",
        juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f },
        0.0f,
        juce::AudioParameterFloatAttributes().withLabel ("dB")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::mix, 1 },
        "Mix",
        juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f },
        100.0f,
        juce::AudioParameterFloatAttributes().withLabel ("%")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::lowCrossover, 1 },
        "Low Split",
        juce::NormalisableRange<float> { 20.0f, 8000.0f, 0.01f, 0.35f },
        120.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { ParamIDs::highCrossover, 1 },
        "High Split",
        juce::NormalisableRange<float> { 80.0f, 16000.0f, 0.01f, 0.35f },
        4000.0f,
        juce::AudioParameterFloatAttributes().withLabel ("Hz")));

    auto addBandParams = [&layout] (const juce::String& prefix,
                                    const char* driveId,
                                    const char* preGainId,
                                    const char* mixId,
                                    const char* gainId,
                                    const char* typeId,
                                    const char* bypassId,
                                    float defaultDrive)
    {
        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { driveId, 1 },
            prefix + " Drive",
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f },
            defaultDrive,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { preGainId, 1 },
            prefix + " Pre",
            juce::NormalisableRange<float> { -24.0f, 24.0f, 0.1f },
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { mixId, 1 },
            prefix + " Mix",
            juce::NormalisableRange<float> { 0.0f, 100.0f, 0.1f },
            100.0f,
            juce::AudioParameterFloatAttributes().withLabel ("%")));

        layout.add (std::make_unique<juce::AudioParameterFloat> (
            juce::ParameterID { gainId, 1 },
            prefix + " Post",
            juce::NormalisableRange<float> { -24.0f, 12.0f, 0.1f },
            0.0f,
            juce::AudioParameterFloatAttributes().withLabel ("dB")));

        layout.add (std::make_unique<juce::AudioParameterChoice> (
            juce::ParameterID { typeId, 1 },
            prefix + " Type",
            ParamChoices::saturationTypes,
            0));

        layout.add (std::make_unique<juce::AudioParameterBool> (
            juce::ParameterID { bypassId, 1 },
            prefix + " Bypass",
            false));
    };

    addBandParams ("Low",  ParamIDs::lowDrive,  ParamIDs::lowPreGain,  ParamIDs::lowMix,  ParamIDs::lowGain,  ParamIDs::lowType,  ParamIDs::lowBypass,  35.0f);
    addBandParams ("Mid",  ParamIDs::midDrive,  ParamIDs::midPreGain,  ParamIDs::midMix,  ParamIDs::midGain,  ParamIDs::midType,  ParamIDs::midBypass,  45.0f);
    addBandParams ("High", ParamIDs::highDrive, ParamIDs::highPreGain, ParamIDs::highMix, ParamIDs::highGain, ParamIDs::highType, ParamIDs::highBypass, 25.0f);

    return layout;
}
