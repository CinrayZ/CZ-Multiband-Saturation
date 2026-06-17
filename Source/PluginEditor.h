#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SpectrumAnalyzer.h"

class BandControlPanel : public juce::Component
{
public:
    using SoloChangeCallback = std::function<void (int bandIndex, bool soloed)>;

    BandControlPanel (juce::AudioProcessorValueTreeState& state,
                      const juce::String& title,
                      const char* driveId,
                      const char* preGainId,
                      const char* mixId,
                      const char* gainId,
                      const char* typeId,
                      const char* bypassId,
                      juce::Colour accentColour,
                      int bandIndex,
                      SoloChangeCallback onSoloChanged);

    void setSoloState (bool soloed);

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    int bandIndex = 0;
    SoloChangeCallback onSoloChanged;
    bool ignoreSoloCallbacks = false;
    juce::Label titleLabel;
    juce::Label driveLabel;
    juce::Label preGainLabel;
    juce::Label mixLabel;
    juce::Label gainLabel;
    juce::Slider driveSlider;
    juce::Slider preGainSlider;
    juce::Slider mixSlider;
    juce::Slider gainSlider;
    juce::ComboBox typeBox;
    juce::ToggleButton soloButton;
    juce::ToggleButton bypassButton;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> driveAttachment;
    std::unique_ptr<SliderAttachment> preGainAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> gainAttachment;
    std::unique_ptr<ComboAttachment> typeAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;

    juce::Colour accent;
};

class MultibandSaturationAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    explicit MultibandSaturationAudioProcessorEditor (MultibandSaturationAudioProcessor&);
    ~MultibandSaturationAudioProcessorEditor() override;

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    MultibandSaturationAudioProcessor& audioProcessor;

    juce::Slider inputGainSlider;
    juce::Slider outputGainSlider;
    juce::Slider mixSlider;
    juce::Slider lowCrossoverSlider;
    juce::Slider highCrossoverSlider;

    juce::Label inputGainLabel;
    juce::Label outputGainLabel;
    juce::Label mixLabel;
    juce::Label lowCrossoverLabel;
    juce::Label highCrossoverLabel;

    std::unique_ptr<BandControlPanel> lowBandPanel;
    std::unique_ptr<BandControlPanel> midBandPanel;
    std::unique_ptr<BandControlPanel> highBandPanel;

    std::unique_ptr<SpectrumDisplay> spectrumDisplay;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    std::unique_ptr<SliderAttachment> inputGainAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> lowCrossoverAttachment;
    std::unique_ptr<SliderAttachment> highCrossoverAttachment;

    void configureGlobalSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void handleBandSoloChanged (int bandIndex, bool soloed);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MultibandSaturationAudioProcessorEditor)
};
