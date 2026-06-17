#include "SpectrumAnalyzer.h"
#include "Parameters.h"

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

    juce::String formatFrequency (float hz)
    {
        if (hz >= 1000.0f)
            return juce::String (hz / 1000.0f, hz >= 10000.0f ? 0 : 1) + " kHz";

        return juce::String (static_cast<int> (hz)) + " Hz";
    }
}

SpectrumDisplay::SpectrumDisplay (MultibandSaturationAudioProcessor& processor,
                                  juce::AudioProcessorValueTreeState& state)
    : audioProcessor (processor),
      apvts (state),
      displayMagnitudes (static_cast<size_t> (MultibandSaturationAudioProcessor::spectrumNumBins), 0.0f)
{
    startTimerHz (30);
}

void SpectrumDisplay::timerCallback()
{
    std::array<float, MultibandSaturationAudioProcessor::spectrumNumBins> newMagnitudes {};
    audioProcessor.getSpectrumMagnitudes (newMagnitudes.data(), static_cast<int> (newMagnitudes.size()));

    for (size_t i = 0; i < displayMagnitudes.size(); ++i)
    {
        const auto target = newMagnitudes[i];
        displayMagnitudes[i] = juce::jmax (target, displayMagnitudes[i] * 0.82f);
    }

    repaint();
}

juce::Rectangle<float> SpectrumDisplay::getPlotBounds() const
{
    return getLocalBounds().toFloat().reduced (12.0f, 8.0f);
}

float SpectrumDisplay::frequencyToX (float frequency) const
{
    const auto plot = getPlotBounds();
    const auto sampleRate = static_cast<float> (audioProcessor.getSampleRateForSpectrum());
    const auto maxHz = juce::jmax (minDisplayHz * 2.0f, sampleRate * 0.5f);
    const auto logMin = std::log10 (minDisplayHz);
    const auto logMax = std::log10 (maxHz);
    const auto proportion = (std::log10 (juce::jlimit (minDisplayHz, maxHz, frequency)) - logMin) / (logMax - logMin);

    return plot.getX() + proportion * plot.getWidth();
}

float SpectrumDisplay::xToFrequency (float x) const
{
    const auto plot = getPlotBounds();
    const auto sampleRate = static_cast<float> (audioProcessor.getSampleRateForSpectrum());
    const auto maxHz = juce::jmax (minDisplayHz * 2.0f, sampleRate * 0.5f);
    const auto logMin = std::log10 (minDisplayHz);
    const auto logMax = std::log10 (maxHz);
    const auto proportion = juce::jlimit (0.0f, 1.0f, (x - plot.getX()) / plot.getWidth());

    return std::pow (10.0f, logMin + proportion * (logMax - logMin));
}

int SpectrumDisplay::hitTestCrossover (float x) const
{
    const auto lowHz = apvts.getRawParameterValue (ParamIDs::lowCrossover)->load();
    const auto highHz = apvts.getRawParameterValue (ParamIDs::highCrossover)->load();
    const auto lowX = frequencyToX (lowHz);
    const auto highX = frequencyToX (highHz);

    if (std::abs (x - lowX) <= static_cast<float> (dragHitPixels))
        return 1;

    if (std::abs (x - highX) <= static_cast<float> (dragHitPixels))
        return 2;

    return 0;
}

void SpectrumDisplay::setCrossoverFrequency (const char* paramId, float frequencyHz)
{
    if (auto* param = apvts.getParameter (paramId))
        param->setValueNotifyingHost (param->convertTo0to1 (frequencyHz));
}

void SpectrumDisplay::mouseDown (const juce::MouseEvent& e)
{
    draggingCrossover = hitTestCrossover (static_cast<float> (e.position.x));
}

void SpectrumDisplay::mouseDrag (const juce::MouseEvent& e)
{
    if (draggingCrossover == 0)
        return;

    const auto frequency = xToFrequency (static_cast<float> (e.position.x));
    const auto lowHz = apvts.getRawParameterValue (ParamIDs::lowCrossover)->load();
    const auto highHz = apvts.getRawParameterValue (ParamIDs::highCrossover)->load();

    if (draggingCrossover == 1)
    {
        const auto clamped = juce::jmin (frequency, highHz - 100.0f);
        setCrossoverFrequency (ParamIDs::lowCrossover, clamped);
    }
    else
    {
        const auto clamped = juce::jmax (frequency, lowHz + 100.0f);
        setCrossoverFrequency (ParamIDs::highCrossover, clamped);
    }
}

void SpectrumDisplay::mouseUp (const juce::MouseEvent&)
{
    draggingCrossover = 0;
}

void SpectrumDisplay::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (juce::Colour (0xff181c26));
    g.fillRoundedRectangle (bounds.reduced (2.0f), 8.0f);

    const auto plot = getPlotBounds();
    const auto sampleRate = static_cast<float> (audioProcessor.getSampleRateForSpectrum());
    const auto maxHz = juce::jmax (minDisplayHz * 2.0f, sampleRate * 0.5f);
    const auto lowHz = apvts.getRawParameterValue (ParamIDs::lowCrossover)->load();
    const auto highHz = apvts.getRawParameterValue (ParamIDs::highCrossover)->load();
    const auto lowX = frequencyToX (lowHz);
    const auto highX = frequencyToX (highHz);

    g.setColour (bandColour (0).withAlpha (0.10f));
    g.fillRect (plot.getX(), plot.getY(), lowX - plot.getX(), plot.getHeight());

    g.setColour (bandColour (1).withAlpha (0.10f));
    g.fillRect (lowX, plot.getY(), highX - lowX, plot.getHeight());

    g.setColour (bandColour (2).withAlpha (0.10f));
    g.fillRect (highX, plot.getY(), plot.getRight() - highX, plot.getHeight());

    static const float gridFrequencies[] { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f };
    g.setColour (juce::Colours::white.withAlpha (0.08f));

    for (const auto freq : gridFrequencies)
    {
        if (freq < minDisplayHz || freq > maxHz)
            continue;

        const auto x = frequencyToX (freq);
        g.drawVerticalLine (static_cast<int> (x), plot.getY(), plot.getBottom());
    }

    const auto fftSize = static_cast<float> (MultibandSaturationAudioProcessor::spectrumFftSize);
    juce::Path spectrumLine;
    juce::Path spectrumFill;
    bool started = false;

    for (int x = static_cast<int> (plot.getX()); x <= static_cast<int> (plot.getRight()); ++x)
    {
        const auto freq = xToFrequency (static_cast<float> (x));
        const auto binIndex = static_cast<int> (freq * fftSize / sampleRate);
        const auto clampedBin = juce::jlimit (0, MultibandSaturationAudioProcessor::spectrumNumBins - 1, binIndex);
        const auto magnitude = displayMagnitudes[static_cast<size_t> (clampedBin)];
        const auto levelDb = juce::Decibels::gainToDecibels (magnitude, -100.0f);
        const auto normalised = juce::jlimit (0.0f, 1.0f, (levelDb + 60.0f) / 60.0f);
        const auto y = plot.getBottom() - normalised * plot.getHeight();

        if (! started)
        {
            spectrumLine.startNewSubPath (static_cast<float> (x), y);
            spectrumFill.startNewSubPath (static_cast<float> (x), y);
            started = true;
        }
        else
        {
            spectrumLine.lineTo (static_cast<float> (x), y);
            spectrumFill.lineTo (static_cast<float> (x), y);
        }
    }

    if (started)
    {
        spectrumFill.lineTo (plot.getRight(), plot.getBottom());
        spectrumFill.lineTo (plot.getX(), plot.getBottom());
        spectrumFill.closeSubPath();

        juce::ColourGradient fill (juce::Colour (0xff8ecae6).withAlpha (0.55f), plot.getTopLeft(),
                                   juce::Colour (0xff219ebc).withAlpha (0.08f), plot.getBottomLeft(), false);
        g.setGradientFill (fill);
        g.fillPath (spectrumFill);

        g.setColour (juce::Colour (0xff90e0ef).withAlpha (0.9f));
        g.strokePath (spectrumLine, juce::PathStrokeType (1.5f));
    }

    const auto drawCrossover = [&g, plot] (float x, juce::Colour colour, const juce::String& label)
    {
        g.setColour (colour.withAlpha (0.95f));
        g.drawLine (x, plot.getY(), x, plot.getBottom(), 2.0f);

        g.setFont (juce::FontOptions (11.0f).withStyle ("Bold"));
        g.drawText (label, juce::Rectangle<float> (x - 36.0f, plot.getY() + 2.0f, 72.0f, 16.0f),
                    juce::Justification::centred);
    };

    drawCrossover (lowX, bandColour (0), formatFrequency (lowHz));
    drawCrossover (highX, bandColour (2), formatFrequency (highHz));

    g.setColour (juce::Colours::white.withAlpha (0.45f));
    g.setFont (juce::FontOptions (11.0f));
    g.drawText ("Spectrum", plot.withHeight (16.0f), juce::Justification::centredLeft);
    g.drawText ("Drag split lines to adjust crossover", plot.withHeight (16.0f), juce::Justification::centredRight);
}
