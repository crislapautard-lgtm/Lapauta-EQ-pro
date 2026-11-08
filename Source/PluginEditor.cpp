
#include "PluginEditor.h"

#include <cmath>
#include <algorithm>

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
}

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

    const float normalized =
        std::log(frequency / 20.0f)
        / std::log(20000.0f / 20.0f);

    return normalized * width;
}

float LaPautaEQProAudioProcessorEditor::EQGraph::gainToY(
    float gain) const
{
    const float height = static_cast<float>(getHeight());

    return height * 0.5f
        - (gain / 24.0f) * height * 0.5f;
}

float LaPautaEQProAudioProcessorEditor::EQGraph::xToFrequency(
    float x) const
{
    const float normalized = juce::jlimit(
        0.0f,
        1.0f,
        x / static_cast<float>(getWidth()));

    return 20.0f * std::pow(1000.0f, normalized);
}

float LaPautaEQProAudioProcessorEditor::EQGraph::yToGain(
    float y) const
{
    const float normalized =
        1.0f - 2.0f * y / static_cast<float>(getHeight());

    return juce::jlimit(-18.0f, 18.0f, normalized * 24.0f);
}

void LaPautaEQProAudioProcessorEditor::EQGraph::paint(
    juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    g.fillAll(backgroundColour);

    g.setColour(panelColour);
    g.fillRoundedRectangle(bounds, 9.0f);

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Horizontal gain grid.

    g.setColour(gridColour);
    g.setFont(11.0f);

    for (int db = -18; db <= 18; db += 6)
    {
        const float y = gainToY(static_cast<float>(db));

        g.drawHorizontalLine(
            static_cast<int>(y),
            0.0f,
            width);

        g.setColour(mutedColour);

        g.drawText(
            juce::String(db),
            5,
            static_cast<int>(y - 10.0f),
            32,
            18,
            juce::Justification::centredLeft);

        g.setColour(gridColour);
    }

    // Frequency grid.

    const float frequencies[] = {
        20.0f, 50.0f, 100.0f, 200.0f,
        500.0f, 1000.0f, 2000.0f,
        5000.0f, 10000.0f, 20000.0f
    };

    for (float frequency : frequencies)
    {
        const float x = frequencyToX(frequency);

        g.setColour(gridColour);

        g.drawVerticalLine(
            static_cast<int>(x),
            0.0f,
            height);

        g.setColour(mutedColour);

        juce::String label;

        if (frequency >= 1000.0f)
            label = juce::String(frequency / 1000.0f, 1) + "k";
        else
            label = juce::String(static_cast<int>(frequency));

        g.drawText(
            label,
            static_cast<int>(x - 23.0f),
            static_cast<int>(height - 23.0f),
            46,
            18,
            juce::Justification::centred);
    }

    // Zero dB reference line.

    g.setColour(juce::Colour(0xff59616c));

    g.drawHorizontalLine(
        static_cast<int>(gainToY(0.0f)),
        0.0f,
        width);

    // Live spectrum analyzer.

    std::array<float, LaPautaEQProAudioProcessor::analyzerSize>
        analyzer{};

    processor.getAnalyzerData(analyzer);

    juce::Path spectrumPath;

    for (int i = 0;
         i < LaPautaEQProAudioProcessor::analyzerSize;
         ++i)
    {
        const float x =
            width * static_cast<float>(i)
            / static_cast<float>(
                LaPautaEQProAudioProcessor::analyzerSize - 1);

        const float magnitude = analyzer[static_cast<size_t>(i)];

        const float y =
            height - 28.0f
            - magnitude * height * 0.62f;

        if (i == 0)
            spectrumPath.startNewSubPath(x, y);
        else
            spectrumPath.lineTo(x, y);
    }

    juce::Path spectrumFill = spectrumPath;

    spectrumFill.lineTo(width, height - 20.0f);
    spectrumFill.lineTo(0.0f, height - 20.0f);
    spectrumFill.closeSubPath();

    g.setColour(blueColour.withAlpha(0.13f));
    g.fillPath(spectrumFill);

    g.setColour(blueColour.withAlpha(0.85f));
    g.strokePath(
        spectrumPath,
        juce::PathStrokeType(2.0f));

    // Combined EQ response.

    juce::Path responsePath;

    for (int x = 0; x <= static_cast<int>(width); x += 2)
    {
        const float frequency =
            xToFrequency(static_cast<float>(x));

        float totalGain = 0.0f;

        for (int band = 0;
             band < LaPautaEQProAudioProcessor::numBands;
             ++band)
        {
            const float center =
                processor.getBandFrequency(band);

            const float gain =
                processor.getBandGain(band);

            const float q =
                processor.getBandQ(band);

            const float octaves =
                std::log2(frequency / center);

            const float shape =
                std::exp(
                    -0.5f * std::pow(
                        octaves * q * 1.5f,
                        2.0f));

            totalGain += gain * shape;
        }

        totalGain = juce::jlimit(
            -18.0f,
            18.0f,
            totalGain);

        const float y = gainToY(totalGain);

        if (x == 0)
            responsePath.startNewSubPath(
                static_cast<float>(x), y);
        else
            responsePath.lineTo(
                static_cast<float>(x), y);
    }

    g.setColour(yellowColour.withAlpha(0.15f));

    juce::Path responseFill = responsePath;
    responseFill.lineTo(width, gainToY(0.0f));
    responseFill.lineTo(0.0f, gainToY(0.0f));
    responseFill.closeSubPath();

    g.fillPath(responseFill);

    g.setColour(yellowColour);

    g.strokePath(
        responsePath,
        juce::PathStrokeType(
            2.8f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Six draggable control points.

    for (int band = 0;
         band < LaPautaEQProAudioProcessor::numBands;
         ++band)
    {
        const float x = frequencyToX(
            processor.getBandFrequency(band));

        const float y = gainToY(
            processor.getBandGain(band));

        const bool selected = band == editor.selectedBand;

        const juce::Colour pointColour =
            selected ? turquoiseColour : yellowColour;

        g.setColour(pointColour.withAlpha(0.18f));
        g.fillEllipse(x - 13.0f, y - 13.0f, 26.0f, 26.0f);

        g.setColour(pointColour);
        g.fillEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f);

        g.setColour(backgroundColour);
        g.drawEllipse(x - 6.0f, y - 6.0f, 12.0f, 12.0f, 1.5f);

        g.setColour(whiteColour);
        g.setFont(10.0f);

        g.drawText(
            juce::String(band + 1),
            static_cast<int>(x - 12.0f),
            static_cast<int>(y - 27.0f),
            24,
            15,
            juce::Justification::centred);
    }
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDown(
    const juce::MouseEvent& event)
{
    const auto position = event.position;

    float closestDistance = 24.0f;
    draggingBand = -1;

    for (int band = 0;
         band < LaPautaEQProAudioProcessor::numBands;
         ++band)
    {
        const float x = frequencyToX(
            processor.getBandFrequency(band));

        const float y = gainToY(
            processor.getBandGain(band));

        const float distance = position.getDistanceFrom(
            { x, y });

        if (distance < closestDistance)
        {
            closestDistance = distance;
            draggingBand = band;
        }
    }

    if (draggingBand >= 0)
    {
        editor.selectBand(draggingBand);
        updateBandFromMouse(draggingBand, position);
    }
}

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDrag(
    const juce::MouseEvent& event)
{
    if (draggingBand < 0)
        return;

    updateBandFromMouse(
        draggingBand,
        event.position);
}

void LaPautaEQProAudioProcessorEditor::EQGraph::updateBandFromMouse(
    int band,
    juce::Point<float> position)
{
    const float frequency = xToFrequency(position.x);
    const float gain = yToGain(position.y);

    processor.setBandFrequency(band, frequency);
    processor.setBandGain(band, gain);

    editor.updateSelectedBand();

    repaint();
}

// ------------------------------------------------------------
// MAIN EDITOR
// ------------------------------------------------------------

LaPautaEQProAudioProcessorEditor::
LaPautaEQProAudioProcessorEditor(
    LaPautaEQProAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      graph(processor, *this)
{
    setSize(1100, 720);

    titleLabel.setText(
        "LA PAUTA EQ PRO",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(
            juce::FontOptions(25.0f).withStyle("Bold")));

    titleLabel.setColour(
        juce::Label::textColourId,
        whiteColour);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "SIX BAND PARAMETRIC EQUALIZER",
        juce::dontSendNotification);

    subtitleLabel.setFont(juce::Font(11.0f));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        mutedColour);

    addAndMakeVisible(subtitleLabel);

    addAndMakeVisible(graph);

    for (int i = 0; i < 6; ++i)
    {
        auto& button = bandButtons[static_cast<size_t>(i)];

        button.setButtonText(
            "BAND " + juce::String(i + 1));

        button.setClickingTogglesState(false);

        button.setColour(
            juce::TextButton::buttonColourId,
            panelColour);

        button.setColour(
            juce::TextButton::textColourOffId,
            whiteColour);

        button.setColour(
            juce::TextButton::textColourOnId,
            turquoiseColour);

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

    outputSlider.setRange(-18.0, 18.0, 0.1);
    outputSlider.setNumDecimalPlacesToDisplay(1);

    frequencyLabel.setText(
        "FREQUENCY",
        juce::dontSendNotification);

    gainLabel.setText(
        "GAIN",
        juce::dontSendNotification);

    qLabel.setText(
        "Q",
        juce::dontSendNotification);

    outputLabel.setText(
        "OUTPUT",
        juce::dontSendNotification);

    juce::Label* labels[] = {
        &frequencyLabel,
        &gainLabel,
        &qLabel,
        &outputLabel
    };

    for (auto* label : labels)
    {
        label->setFont(
            juce::Font(
                juce::FontOptions(12.0f).withStyle("Bold")));

        label->setColour(
            juce::Label::textColourId,
            mutedColour);

        label->setJustificationType(
            juce::Justification::centred);

        addAndMakeVisible(*label);
    }

    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(qSlider);
    addAndMakeVisible(outputSlider);

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
        juce::Slider::TextBoxBelow,
        false,
        90,
        22);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        turquoiseColour);

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        gridColour);

    slider.setColour(
        juce::Slider::thumbColourId,
        yellowColour);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        whiteColour);

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        panelColour);

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        gridColour);
}

void LaPautaEQProAudioProcessorEditor::selectBand(int band)
{
    selectedBand = juce::jlimit(0, 5, band);

    frequencyAttachment.reset();
    gainAttachment.reset();
    qAttachment.reset();

    const juce::String suffix =
        juce::String(selectedBand + 1);

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

    for (int i = 0; i < 6; ++i)
    {
        const bool selected = i == selectedBand;

        bandButtons[static_cast<size_t>(i)].setColour(
            juce::TextButton::buttonColourId,
            selected
                ? juce::Colour(0xff263c3b)
                : panelColour);

        bandButtons[static_cast<size_t>(i)].setColour(
            juce::TextButton::textColourOffId,
            selected ? turquoiseColour : whiteColour);
    }

    updateSelectedBand();
    graph.repaint();
}

void LaPautaEQProAudioProcessorEditor::updateSelectedBand()
{
    graph.repaint();
}

void LaPautaEQProAudioProcessorEditor::timerCallback()
{
    graph.repaint();
}

void LaPautaEQProAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(backgroundColour);

    g.setColour(turquoiseColour);
    g.fillRoundedRectangle(
        30.0f, 28.0f, 5.0f, 35.0f, 2.0f);

    g.setColour(panelColour);
    g.drawHorizontalLine(
        100,
        25.0f,
        1075.0f);

    g.setColour(mutedColour);
    g.setFont(11.0f);

    g.drawText(
        "REAL TIME SPECTRUM  |  PARAMETRIC EQ",
        720,
        42,
        350,
        20,
        juce::Justification::centredRight);

    g.setColour(panelColour);
    g.fillRoundedRectangle(
        25.0f, 540.0f, 1050.0f, 155.0f, 10.0f);

    g.setColour(turquoiseColour);
    g.setFont(11.0f);

    g.drawText(
        "BAND CONTROLS",
        40,
        545,
        200,
        18,
        juce::Justification::centredLeft);
}

void LaPautaEQProAudioProcessorEditor::resized()
{
    titleLabel.setBounds(45, 25, 350, 35);
    subtitleLabel.setBounds(47, 62, 350, 20);

    graph.setBounds(35, 115, 1030, 405);

    for (int i = 0; i < 6; ++i)
    {
        bandButtons[static_cast<size_t>(i)].setBounds(
            40 + i * 105,
            75,
            95,
            28);
    }

    frequencyLabel.setBounds(55, 568, 180, 20);
    gainLabel.setBounds(280, 568, 180, 20);
    qLabel.setBounds(505, 568, 180, 20);
    outputLabel.setBounds(790, 568, 180, 20);

    frequencySlider.setBounds(55, 585, 180, 105);
    gainSlider.setBounds(280, 585, 180, 105);
    qSlider.setBounds(505, 585, 180, 105);
    outputSlider.setBounds(790, 585, 180, 105);
}
