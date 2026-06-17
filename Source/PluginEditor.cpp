#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    juce::Colour bandColour (int index)
    {
        static const juce::Colour colours[] {
            juce::Colour (0xffe07a5f),
            juce::Colour (0xff81b29a),
            juce::Colour (0xff3d85c6)
        };

        return colours[juce::jlimit (0, 2, index)];
    }

    void styleRotarySlider (juce::Slider& slider, juce::Colour accent)
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 70, 18);
        slider.setColour (juce::Slider::rotarySliderFillColourId, accent);
        slider.setColour (juce::Slider::thumbColourId, accent.brighter (0.25f));
    }

    void configureBandLabel (juce::Label& label, const juce::String& text)
    {
        label.setText (text, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setFont (juce::FontOptions (12.0f));
        label.setColour (juce::Label::textColourId, juce::Colours::white.withAlpha (0.72f));
    }
}

BandControlPanel::BandControlPanel (juce::AudioProcessorValueTreeState& state,
                                    const juce::String& title,
                                    const char* driveId,
                                    const char* preGainId,
                                    const char* mixId,
                                    const char* gainId,
                                    const char* typeId,
                                    const char* bypassId,
                                    juce::Colour accentColour,
                                    int bandIndexIn,
                                    SoloChangeCallback onSoloChangedIn)
    : bandIndex (bandIndexIn),
      onSoloChanged (std::move (onSoloChangedIn)),
      accent (accentColour)
{
    titleLabel.setText (title, juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (16.0f).withStyle ("Bold"));
    addAndMakeVisible (titleLabel);

    configureBandLabel (driveLabel, "Drive");
    configureBandLabel (preGainLabel, "Pre");
    configureBandLabel (mixLabel, "Mix");
    configureBandLabel (gainLabel, "Post");
    addAndMakeVisible (driveLabel);
    addAndMakeVisible (preGainLabel);
    addAndMakeVisible (mixLabel);
    addAndMakeVisible (gainLabel);

    for (auto* slider : { &driveSlider, &preGainSlider, &mixSlider, &gainSlider })
    {
        styleRotarySlider (*slider, accent);
        addAndMakeVisible (*slider);
    }

    typeBox.addItemList (ParamChoices::saturationTypes, 1);
    addAndMakeVisible (typeBox);

    soloButton.setButtonText ("Solo");
    soloButton.setClickingTogglesState (true);
    soloButton.setColour (juce::TextButton::buttonOnColourId, accent.withAlpha (0.85f));
    soloButton.setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    soloButton.onClick = [this]
    {
        if (ignoreSoloCallbacks || onSoloChanged == nullptr)
            return;

        onSoloChanged (bandIndex, soloButton.getToggleState());
    };
    addAndMakeVisible (soloButton);

    bypassButton.setButtonText ("Bypass");
    addAndMakeVisible (bypassButton);

    driveAttachment   = std::make_unique<SliderAttachment> (state, driveId, driveSlider);
    preGainAttachment = std::make_unique<SliderAttachment> (state, preGainId, preGainSlider);
    mixAttachment     = std::make_unique<SliderAttachment> (state, mixId, mixSlider);
    gainAttachment    = std::make_unique<SliderAttachment> (state, gainId, gainSlider);
    typeAttachment  = std::make_unique<ComboAttachment> (state, typeId, typeBox);
    bypassAttachment = std::make_unique<ButtonAttachment> (state, bypassId, bypassButton);
}

void BandControlPanel::setSoloState (bool soloed)
{
    ignoreSoloCallbacks = true;
    soloButton.setToggleState (soloed, juce::dontSendNotification);
    ignoreSoloCallbacks = false;
}

void BandControlPanel::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat().reduced (2.0f);
    g.setColour (juce::Colour (0xff1f2430));
    g.fillRoundedRectangle (bounds, 8.0f);

    g.setColour (accent.withAlpha (0.85f));
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);
}

void BandControlPanel::resized()
{
    auto area = getLocalBounds().reduced (10);
    titleLabel.setBounds (area.removeFromTop (24));

    auto bottomRow = area.removeFromBottom (28);
    bypassButton.setBounds (bottomRow.removeFromRight (90));
    soloButton.setBounds (bottomRow.removeFromRight (70));
    typeBox.setBounds (bottomRow.reduced (4, 0));

    auto sliderArea = area;
    const int sliderWidth = sliderArea.getWidth() / 4;

    auto labelRow = sliderArea.removeFromTop (16);
    driveLabel.setBounds (labelRow.removeFromLeft (sliderWidth));
    preGainLabel.setBounds (labelRow.removeFromLeft (sliderWidth));
    mixLabel.setBounds (labelRow.removeFromLeft (sliderWidth));
    gainLabel.setBounds (labelRow);

    driveSlider.setBounds (sliderArea.removeFromLeft (sliderWidth).reduced (4));
    preGainSlider.setBounds (sliderArea.removeFromLeft (sliderWidth).reduced (4));
    mixSlider.setBounds (sliderArea.removeFromLeft (sliderWidth).reduced (4));
    gainSlider.setBounds (sliderArea.reduced (4));
}

MultibandSaturationAudioProcessorEditor::MultibandSaturationAudioProcessorEditor (MultibandSaturationAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    auto& apvts = audioProcessor.apvts;

    configureGlobalSlider (inputGainSlider, inputGainLabel, "Input");
    configureGlobalSlider (outputGainSlider, outputGainLabel, "Output");
    configureGlobalSlider (mixSlider, mixLabel, "Mix");
    configureGlobalSlider (lowCrossoverSlider, lowCrossoverLabel, "Low Split");
    configureGlobalSlider (highCrossoverSlider, highCrossoverLabel, "High Split");

    lowCrossoverSlider.setSkewFactorFromMidPoint (400.0f);
    highCrossoverSlider.setSkewFactorFromMidPoint (1200.0f);

    inputGainAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::inputGain, inputGainSlider);
    outputGainAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::outputGain, outputGainSlider);
    mixAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::mix, mixSlider);
    lowCrossoverAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::lowCrossover, lowCrossoverSlider);
    highCrossoverAttachment = std::make_unique<SliderAttachment> (apvts, ParamIDs::highCrossover, highCrossoverSlider);

    const auto handleSoloChanged = [this] (int bandIndex, bool soloed)
    {
        handleBandSoloChanged (bandIndex, soloed);
    };

    lowBandPanel = std::make_unique<BandControlPanel> (apvts, "Low", ParamIDs::lowDrive, ParamIDs::lowPreGain,
                                                       ParamIDs::lowMix, ParamIDs::lowGain, ParamIDs::lowType, ParamIDs::lowBypass,
                                                       bandColour (0), 0, handleSoloChanged);
    midBandPanel = std::make_unique<BandControlPanel> (apvts, "Mid", ParamIDs::midDrive, ParamIDs::midPreGain,
                                                       ParamIDs::midMix, ParamIDs::midGain, ParamIDs::midType, ParamIDs::midBypass,
                                                       bandColour (1), 1, handleSoloChanged);
    highBandPanel = std::make_unique<BandControlPanel> (apvts, "High", ParamIDs::highDrive, ParamIDs::highPreGain,
                                                        ParamIDs::highMix, ParamIDs::highGain, ParamIDs::highType, ParamIDs::highBypass,
                                                        bandColour (2), 2, handleSoloChanged);

    addAndMakeVisible (*lowBandPanel);
    addAndMakeVisible (*midBandPanel);
    addAndMakeVisible (*highBandPanel);

    spectrumDisplay = std::make_unique<SpectrumDisplay> (audioProcessor, apvts);
    addAndMakeVisible (*spectrumDisplay);

    setSize (960, 680);
}

MultibandSaturationAudioProcessorEditor::~MultibandSaturationAudioProcessorEditor() = default;

void MultibandSaturationAudioProcessorEditor::configureGlobalSlider (juce::Slider& slider,
                                                                      juce::Label& label,
                                                                      const juce::String& text)
{
    styleRotarySlider (slider, juce::Colour (0xfff2cc8f));
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (slider);
    addAndMakeVisible (label);
}

void MultibandSaturationAudioProcessorEditor::handleBandSoloChanged (int bandIndex, bool soloed)
{
    if (soloed)
    {
        audioProcessor.setSoloBand (bandIndex);
        lowBandPanel->setSoloState (bandIndex == 0);
        midBandPanel->setSoloState (bandIndex == 1);
        highBandPanel->setSoloState (bandIndex == 2);
    }
    else if (audioProcessor.getSoloBand() == bandIndex)
    {
        audioProcessor.setSoloBand (-1);
        lowBandPanel->setSoloState (false);
        midBandPanel->setSoloState (false);
        highBandPanel->setSoloState (false);
    }
}

void MultibandSaturationAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff12151c));

    auto header = getLocalBounds().removeFromTop (72).toFloat();
    juce::ColourGradient gradient (juce::Colour (0xff2a3140), header.getTopLeft(),
                                   juce::Colour (0xff12151c), header.getBottomLeft(), false);
    g.setGradientFill (gradient);
    g.fillRect (header);

    g.setColour (juce::Colours::white.withAlpha (0.92f));
    g.setFont (juce::FontOptions (24.0f).withStyle ("Bold"));
    g.drawText ("Multiband Saturation", header.reduced (20.0f, 0.0f), juce::Justification::centredLeft);

    g.setFont (juce::FontOptions (13.0f));
    g.setColour (juce::Colours::white.withAlpha (0.55f));
    g.drawText ("3-band crossover with per-band drive and saturation types",
                header.reduced (20.0f, 0.0f),
                juce::Justification::centredRight);
}

void MultibandSaturationAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    area.removeFromTop (72);

    auto spectrumRow = area.removeFromTop (170).reduced (16, 6);
    spectrumDisplay->setBounds (spectrumRow);

    auto globalRow = area.removeFromTop (130).reduced (16, 8);
    const int globalWidth = globalRow.getWidth() / 5;

    auto placeGlobal = [&globalRow, globalWidth] (juce::Label& label, juce::Slider& slider)
    {
        auto slot = globalRow.removeFromLeft (globalWidth).reduced (6);
        label.setBounds (slot.removeFromTop (18));
        slider.setBounds (slot);
    };

    placeGlobal (inputGainLabel, inputGainSlider);
    placeGlobal (outputGainLabel, outputGainSlider);
    placeGlobal (mixLabel, mixSlider);
    placeGlobal (lowCrossoverLabel, lowCrossoverSlider);
    placeGlobal (highCrossoverLabel, highCrossoverSlider);

    auto bandRow = area.reduced (16, 8);
    const int bandWidth = bandRow.getWidth() / 3;

    lowBandPanel->setBounds (bandRow.removeFromLeft (bandWidth).reduced (6));
    midBandPanel->setBounds (bandRow.removeFromLeft (bandWidth).reduced (6));
    highBandPanel->setBounds (bandRow.reduced (6));
}
