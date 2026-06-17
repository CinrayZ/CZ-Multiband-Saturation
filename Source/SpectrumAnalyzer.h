#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumDisplay : public juce::Component,
                        private juce::Timer
{
public:
    SpectrumDisplay (MultibandSaturationAudioProcessor& processor,
                     juce::AudioProcessorValueTreeState& state);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void timerCallback() override;

    float frequencyToX (float frequency) const;
    float xToFrequency (float x) const;
    int hitTestCrossover (float x) const;
    void setCrossoverFrequency (const char* paramId, float frequencyHz);
    juce::Rectangle<float> getPlotBounds() const;

    MultibandSaturationAudioProcessor& audioProcessor;
    juce::AudioProcessorValueTreeState& apvts;

    std::vector<float> displayMagnitudes;
    int draggingCrossover = 0;

    static constexpr float minDisplayHz = 20.0f;
    static constexpr int dragHitPixels = 10;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpectrumDisplay)
};
