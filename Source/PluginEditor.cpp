
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>
#include <array>

namespace
{
    const juce::Colour backgroundColour(0xff090b0e);
    const juce::Colour panelColour(0xff15191f);
    const juce::Colour gridColour(0xff29313a);
    const juce::Colour yellowColour(0xffffd447);
    const juce::Colour blueColour(0xff278dff);
    const juce::Colour turquoiseColour(0xff35d6c5);
    const juce::Colour whiteColour(0xffeeeeee);
    const juce::Colour mutedColour(0xff89939e);

    float getBandParameter(
        LaPautaEQProAudioProcessor& processor,
        const juce::String& id)
    {
        auto* parameter = processor.parameters.getRawParameterValue(id);
        return parameter != nullptr ? parameter->load() : 0.0f;
    }

    void setBandParameter(
        LaPautaEQProAudioProcessor& processor,
        const juce::String& id,
        float value)
    {
        auto* parameter = processor.parameters.getParameter(id);

        if (parameter != nullptr)
        {
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(value));
        }
    }

    juce::String bandSuffix(int band)
    {
        return juce::String(band + 1);
    }

    using EQCoefficients = juce::dsp::IIR::Coefficients<float>;

    EQCoefficients::Ptr makeGraphCoefficients(
        int type,
        double sampleRate,
        float frequency,
        float gain,
        float q)
    {
        if (sampleRate <= 0.0)
            return {};

        const float safeFrequency = juce::jlimit(
            20.0f,
            static_cast<float>(sampleRate * 0.45),
            frequency);

        const float safeQ = juce::jlimit(0.1f, 10.0f, q);

        const float gainFactor =
            juce::Decibels::decibelsToGain(gain);

        switch (type)
        {
            case 1:
                return EQCoefficients::makeLowPass(
                    sampleRate, safeFrequency, safeQ);

            case 2:
                return EQCoefficients::makeBandPass(
                    sampleRate, safeFrequency, safeQ);

            case 3:
                return EQCoefficients::makeHighPass(
                    sampleRate, safeFrequency, safeQ);

            case 4:
                return EQCoefficients::makeNotchFilter(
                    sampleRate, safeFrequency, safeQ);

            case 5:
                return EQCoefficients::makeLowShelf(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            case 6:
                return EQCoefficients::makePeakFilter(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            case 7:
                return EQCoefficients::makeHighShelf(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            default:
                return {};
        }
    }
}

// ============================================================
// EQ GRAPH
// ============================================================

LaPautaEQProAudioProcessorEditor::EQGraph::EQGraph(
    LaPautaEQProAudioProcessor& p,
    LaPautaEQProAudioProcessorEditor& e)
    : processor(p), editor(e)
{
}

float LaPautaEQProAudioProcessorEditor::EQGraph::frequencyToX(
    float frequency) const
{
    const float width = static_cast<float>(getWidth());

    if (width <= 0.0f)
        return 0.0f;

    const float normalized =
        std::log(juce::jlimit(20.0f, 20000.0f, frequency) / 20.0f)
        / std::log(1000.0f);

    return normalized * width;
}

float LaPautaEQProAudioProcessorEditor::EQGraph::gainToY(
    float gain) const
{
    const float height = static_cast<float>(getHeight());

    return height * 0.5f - (gain / 24.0f) * height * 0.5f;
}

float LaPautaEQProAudioProcessorEditor::EQGraph::xToFrequency(
    float x) const
{
    const float width = static_cast<float>(juce::jmax(1, getWidth()));

    const float normalized = juce::jlimit(
        0.0f, 1.0f, x / width);

    return 20.0f * std::pow(1000.0f, normalized);
}

float LaPautaEQProAudioProcessorEditor::EQGraph::yToGain(
    float y) const
{
    const float height = static_cast<float>(juce::jmax(1, getHeight()));

    const float normalized = 1.0f - 2.0f * y / height;

    return juce::jlimit(-18.0f, 18.0f, normalized * 24.0f);
}

int LaPautaEQProAudioProcessorEditor::EQGraph::findClosestBand(
    juce::Point<float> position) const
{
    float closestDistance = 30.0f;
    int closestBand = -1;

    for (int band = 0;
         band < LaPautaEQProAudioProcessor::numBands;
         ++band)
    {
        const auto suffix = bandSuffix(band);

        const float x = frequencyToX(
            getBandParameter(processor, "FREQ" + suffix));

        const float y = gainToY(
            getBandParameter(processor, "GAIN" + suffix));

        const float distance = position.getDistanceFrom(
            juce::Point<float>(x, y));

        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestBand = band;
        }
    }

    return closestBand;
}

void LaPautaEQProAudioProcessorEditor::EQGraph::paint(
    juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(backgroundColour);
    g.setColour(panelColour);
    g.fillRoundedRectangle(bounds, 9.0f);

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Gain grid
    g.setFont(juce::FontOptions(10.0f));
    g.setColour(gridColour);

    for (int db = -18; db <= 18; db += 6)
    {
        const float y = gainToY(static_cast<float>(db));

        g.drawHorizontalLine(static_cast<int>(y), 0.0f, width);

        g.setColour(mutedColour);
        g.drawText(
            juce::String(db),
            5,
            static_cast<int>(y - 9.0f),
            30,
            17,
            juce::Justification::centredLeft);

        g.setColour(gridColour);
    }

    // Frequency grid
    const float frequencies[] = {
        20.0f, 50.0f, 100.0f, 200.0f,
        500.0f, 1000.0f, 2000.0f,
        5000.0f, 10000.0f, 20000.0f
    };

    for (float frequency : frequencies)
    {
        const float x = frequencyToX(frequency);

        g.setColour(gridColour);
        g.drawVerticalLine(static_cast<int>(x), 0.0f, height);

        g.setColour(mutedColour);

        juce::String label;

        if (frequency >= 1000.0f)
            label = juce::String(frequency / 1000.0f, 1) + "k";
        else
            label = juce::String(static_cast<int>(frequency));

        g.drawText(
            label,
            static_cast<int>(x - 22.0f),
            static_cast<int>(height - 20.0f),
            44,
            16,
            juce::Justification::centred);
    }

    // Zero dB line
    g.setColour(juce::Colour(0xff59616c));
    g.drawHorizontalLine(
        static_cast<int>(gainToY(0.0f)), 0.0f, width);

    // Spectrum analyzer
    juce::Path spectrumPath;

    if (processor.getSampleRate() > 0.0)
    {
        for (int i = 0; i <= 240; ++i)
        {
            const float x = width * static_cast<float>(i) / 240.0f;
            const float frequency = xToFrequency(x);

            const int fftBin = juce::jlimit(
                0,
                511,
                static_cast<int>(
                    frequency * 1024.0 / processor.getSampleRate()));

            const float magnitude = processor.getSpectrumValue(fftBin);

            const float db = juce::Decibels::gainToDecibels(
                magnitude / 512.0f, -90.0f);

            const float normalized = juce::jlimit(
                0.0f, 1.0f, (db + 90.0f) / 90.0f);

            const float y =
                height - 24.0f - normalized * height * 0.65f;

            if (i == 0)
                spectrumPath.startNewSubPath(x, y);
            else
                spectrumPath.lineTo(x, y);
        }
    }

    juce::Path spectrumFill = spectrumPath;
    spectrumFill.lineTo(width, height - 18.0f);
    spectrumFill.lineTo(0.0f, height - 18.0f);
    spectrumFill.closeSubPath();

    g.setColour(blueColour.withAlpha(0.13f));
    g.fillPath(spectrumFill);

    g.setColour(blueColour.withAlpha(0.85f));
    g.strokePath(spectrumPath, juce::PathStrokeType(1.8f));

    // Build each band's real filter response.
    std::array<EQCoefficients::Ptr, 6> bandCoefficients {};

    const double sampleRate = processor.getSampleRate();

    for (int band = 0; band < 6; ++band)
    {
        const auto suffix = bandSuffix(band);

        const int type = juce::roundToInt(
            getBandParameter(processor, "TYPE" + suffix));

        const bool enabled =
            getBandParameter(processor, "BANDON" + suffix) > 0.5f;

        if (!enabled || type == 0)
            continue;

        bandCoefficients[static_cast<size_t>(band)] =
            makeGraphCoefficients(
                type,
                sampleRate,
                getBandParameter(processor, "FREQ" + suffix),
                getBandParameter(processor, "GAIN" + suffix),
                getBandParameter(processor, "Q" + suffix));
    }

    // EQ response curve
    juce::Path responsePath;

    for (int pixel = 0; pixel <= static_cast<int>(width); pixel += 2)
    {
        const float frequency =
            xToFrequency(static_cast<float>(pixel));

        float totalGain = 0.0f;

        for (int band = 0; band < 6; ++band)
        {
            const auto& coeff =
                bandCoefficients[static_cast<size_t>(band)];

            if (coeff == nullptr)
                continue;

            const float magnitude =
                coeff->getMagnitudeForFrequency(frequency, sampleRate);

            totalGain += juce::Decibels::gainToDecibels(
                magnitude, -120.0f);
        }

        totalGain = juce::jlimit(-36.0f, 36.0f, totalGain);

        const float y = gainToY(totalGain);

        if (pixel == 0)
            responsePath.startNewSubPath(
                static_cast<float>(pixel), y);
        else
            responsePath.lineTo(static_cast<float>(pixel), y);
    }

    juce::Path responseFill = responsePath;
    responseFill.lineTo(width, gainToY(0.0f));
    responseFill.lineTo(0.0f, gainToY(0.0f));
    responseFill.closeSubPath();

    g.setColour(yellowColour.withAlpha(0.15f));
    g.fillPath(responseFill);

    g.setColour(yellowColour);
    g.strokePath(responsePath, juce::PathStrokeType(2.5f));

    // Six control points
    for (int band = 0; band < 6; ++band)
    {
        const auto suffix = bandSuffix(band);

        const float x = frequencyToX(
            getBandParameter(processor, "FREQ" + suffix));

        const float y = gainToY(
            getBandParameter(processor, "GAIN" + suffix));

        const bool enabled =
            getBandParameter(processor, "BANDON" + suffix) > 0.5f
            && getBandParameter(processor, "TYPE" + suffix) > 0.5f;

        const bool selected = band == editor.selectedBand;

        juce::Colour pointColour =
            selected ? turquoiseColour : yellowColour;

        if (!enabled)
            pointColour = mutedColour;

        g.setColour(pointColour.withAlpha(enabled ? 0.18f : 0.08f));
        g.fillEllipse(x - 12.0f, y - 12.0f, 24.0f, 24.0f);

        g.setColour(pointColour);
        g.fillEllipse(x - 5.5f, y - 5.5f, 11.0f, 11.0f);

        g.setColour(backgroundColour);
        g.drawEllipse(x - 5.5f, y - 5.5f, 11.0f, 11.0f, 1.3f);

        g.setColour(whiteColour);
        g.setFont(juce::FontOptions(9.0f));

        g.drawText(
            juce::String(band + 1),
            static_cast<int>(x - 11.0f),
            static_cast<int>(y - 24.0f),
            22,
            14,
            juce::Justification::centred);
    }
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDown(
    const juce::MouseEvent& event)
{
    draggingBand = findClosestBand(event.position);
    dragStarted = false;
    mouseDownPosition = event.position;

    if (draggingBand >= 0)
        editor.selectBand(draggingBand);
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDrag(
    const juce::MouseEvent& event)
{
    if (draggingBand < 0)
        return;

    if (event.position.getDistanceFrom(mouseDownPosition) > 2.0f)
        dragStarted = true;

    if (dragStarted)
        updateBandFromMouse(draggingBand, event.position);
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseUp(
    const juce::MouseEvent&)
{
    if (draggingBand >= 0 && !dragStarted)
    {
        const auto suffix = bandSuffix(draggingBand);

        const bool currentlyEnabled =
            getBandParameter(processor, "BANDON" + suffix) > 0.5f;

        setBandParameter(
            processor,
            "BANDON" + suffix,
            currentlyEnabled ? 0.0f : 1.0f);

        editor.updateSelectedBand();
    }

    draggingBand = -1;
    dragStarted = false;
    repaint();
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseWheelMove(
    const juce::MouseEvent& event,
    const juce::MouseWheelDetails& wheel)
{
    int band = findClosestBand(event.position);

    if (band < 0)
        band = editor.selectedBand;

    const auto suffix = bandSuffix(band);

    const float currentGain =
        getBandParameter(processor, "GAIN" + suffix);

    const float newGain = juce::jlimit(
        -18.0f,
        18.0f,
        currentGain + wheel.deltaY * 3.0f);

    setBandParameter(
        processor,
        "GAIN" + suffix,
        newGain);

    editor.selectBand(band);
    editor.updateSelectedBand();
    repaint();
}

void LaPautaEQProAudioProcessorEditor::EQGraph::updateBandFromMouse(
    int band,
    juce::Point<float> position)
{
    const auto suffix = bandSuffix(band);

    setBandParameter(
        processor, "FREQ" + suffix, xToFrequency(position.x));

    setBandParameter(
        processor, "GAIN" + suffix, yToGain(position.y));

    editor.updateSelectedBand();
    repaint();
}

// ============================================================
// MAIN EDITOR
// ============================================================

LaPautaEQProAudioProcessorEditor::
LaPautaEQProAudioProcessorEditor(
    LaPautaEQProAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      graph(processor, *this)
{
    // Fixed window size.
    setSize(900, 550);

    titleLabel.setText("LA PAUTA EQ PRO", juce::dontSendNotification);
    titleLabel.setFont(
        juce::FontOptions(23.0f).withStyle("Bold"));
    titleLabel.setColour(
        juce::Label::textColourId, whiteColour);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "SIX BAND PARAMETRIC EQUALIZER",
        juce::dontSendNotification);
    subtitleLabel.setFont(juce::FontOptions(10.0f));
    subtitleLabel.setColour(
        juce::Label::textColourId, mutedColour);
    addAndMakeVisible(subtitleLabel);

    addAndMakeVisible(graph);

    for (int i = 0; i < 6; ++i)
    {
        auto& button = bandButtons[static_cast<size_t>(i)];

        button.setButtonText("BAND " + juce::String(i + 1));
        button.setClickingTogglesState(false);

        button.setColour(
            juce::TextButton::buttonColourId, panelColour);
        button.setColour(
            juce::TextButton::textColourOffId, whiteColour);
        button.setColour(
            juce::TextButton::textColourOnId, turquoiseColour);

        button.onClick = [this, i]()
        {
            selectBand(i);
        };

        addAndMakeVisible(button);
    }

    styleSlider(frequencySlider);
    styleSlider(gainSlider);
    styleSlider(qSlider);
    styleSlider(outputSlider);

    frequencySlider.setRange(20.0, 20000.0, 1.0);
    frequencySlider.setSkewFactorFromMidPoint(1000.0);
    frequencySlider.setNumDecimalPlacesToDisplay(0);

    gainSlider.setRange(-18.0, 18.0, 0.1);
    gainSlider.setNumDecimalPlacesToDisplay(1);

    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setNumDecimalPlacesToDisplay(2);

    outputSlider.setRange(-24.0, 12.0, 0.1);
    outputSlider.setNumDecimalPlacesToDisplay(1);

    frequencyLabel.setText("FREQUENCY", juce::dontSendNotification);
    gainLabel.setText("GAIN", juce::dontSendNotification);
    qLabel.setText("Q", juce::dontSendNotification);
    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    filterTypeLabel.setText("FILTER TYPE", juce::dontSendNotification);

    juce::Label* labels[] = {
        &frequencyLabel,
        &gainLabel,
        &qLabel,
        &outputLabel,
        &filterTypeLabel
    };

    for (auto* label : labels)
    {
        label->setFont(
            juce::FontOptions(10.0f).withStyle("Bold"));
        label->setColour(
            juce::Label::textColourId, mutedColour);
        label->setJustificationType(
            juce::Justification::centred);
        addAndMakeVisible(*label);
    }

    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(qSlider);
    addAndMakeVisible(outputSlider);

    filterTypeBox.addItem("Disabled", 1);
    filterTypeBox.addItem("Low pass", 2);
    filterTypeBox.addItem("Band pass", 3);
    filterTypeBox.addItem("High pass", 4);
    filterTypeBox.addItem("Band stop", 5);
    filterTypeBox.addItem("Low shelf", 6);
    filterTypeBox.addItem("Peaking", 7);
    filterTypeBox.addItem("High shelf", 8);

    filterTypeBox.setColour(
        juce::ComboBox::backgroundColourId, panelColour);
    filterTypeBox.setColour(
        juce::ComboBox::textColourId, whiteColour);
    filterTypeBox.setColour(
        juce::ComboBox::outlineColourId, gridColour);
    filterTypeBox.setColour(
        juce::ComboBox::arrowColourId, turquoiseColour);

    filterTypeBox.onChange = [this]()
    {
        const float type = static_cast<float>(
            juce::jmax(0, filterTypeBox.getSelectedId() - 1));

        setBandParameter(
            audioProcessor,
            "BANDON" + bandSuffix(selectedBand),
            type > 0.0f ? 1.0f : 0.0f);

        updateSelectedBand();
    };

    addAndMakeVisible(filterTypeBox);

    outputAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "OUTPUT",
                outputSlider);

    selectBand(0);
    startTimerHz(30);
}

LaPautaEQProAudioProcessorEditor::
~LaPautaEQProAudioProcessorEditor()
{
    stopTimer();
}

void LaPautaEQProAudioProcessorEditor::styleSlider(
    juce::Slider& slider)
{
    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 75, 19);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId, turquoiseColour);
    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId, gridColour);
    slider.setColour(
        juce::Slider::thumbColourId, yellowColour);
    slider.setColour(
        juce::Slider::textBoxTextColourId, whiteColour);
    slider.setColour(
        juce::Slider::textBoxBackgroundColourId, panelColour);
    slider.setColour(
        juce::Slider::textBoxOutlineColourId, gridColour);
}

void LaPautaEQProAudioProcessorEditor::selectBand(int band)
{
    selectedBand = juce::jlimit(0, 5, band);

    frequencyAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();
    filterTypeAttachment.reset();

    const juce::String suffix = bandSuffix(selectedBand);

    frequencyAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "FREQ" + suffix,
                frequencySlider);

    gainAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "GAIN" + suffix,
                gainSlider);

    qAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "Q" + suffix,
                qSlider);

    filterTypeAttachment =
        std::make_unique<
    
