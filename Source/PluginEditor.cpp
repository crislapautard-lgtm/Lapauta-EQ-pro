
#include "PluginEditor.h"
#include <cmath>

namespace
{
    const juce::Colour backgroundColour(17, 20, 25);
    const juce::Colour panelColour(25, 30, 37);
    const juce::Colour turquoise(0, 220, 190);
    const juce::Colour yellow(255, 205, 55);
    const juce::Colour mutedText(145, 158, 170);

    const std::array<juce::Colour, 6> bandColours
    {
        juce::Colour(255, 100, 100),
        juce::Colour(255, 170, 65),
        juce::Colour(235, 220, 70),
        juce::Colour(80, 220, 145),
        juce::Colour(65, 175, 255),
        juce::Colour(190, 120, 255)
    };

    juce::String parameterID(const juce::String& prefix, int band)
    {
        return prefix + juce::String(band + 1);
    }

    void setParameter(
        juce::AudioProcessorValueTreeState& state,
        const juce::String& id,
        float value)
    {
        if (auto* parameter = state.getParameter(id))
        {
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(value));
        }
    }
}

//==============================================================================
// EQ Graph
//==============================================================================

LaPautaEQProAudioProcessorEditor::EQGraph::EQGraph(
    LaPautaEQProAudioProcessor& p,
    LaPautaEQProAudioProcessorEditor& e)
    : processor(p), editor(e)
{
    setInterceptsMouseClicks(true, true);
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::frequencyToX(
    float frequency) const
{
    const float width = static_cast<float>(getWidth());

    const float minFrequency = 20.0f;
    const float maxFrequency = 20000.0f;

    const float normalized =
        std::log(juce::jlimit(minFrequency, maxFrequency, frequency)
                 / minFrequency)
        / std::log(maxFrequency / minFrequency);

    return normalized * width;
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::gainToY(
    float gain) const
{
    const float height = static_cast<float>(getHeight());

    const float normalized =
        (18.0f - juce::jlimit(-18.0f, 18.0f, gain)) / 36.0f;

    return normalized * height;
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::xToFrequency(
    float x) const
{
    const float width = juce::jmax(1.0f, static_cast<float>(getWidth()));

    const float normalized = juce::jlimit(0.0f, 1.0f, x / width);

    return 20.0f * std::pow(1000.0f, normalized);
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::yToGain(
    float y) const
{
    const float height = juce::jmax(1.0f, static_cast<float>(getHeight()));

    const float normalized = juce::jlimit(0.0f, 1.0f, y / height);

    return 18.0f - normalized * 36.0f;
}

//==============================================================================

int LaPautaEQProAudioProcessorEditor::EQGraph::findClosestBand(
    juce::Point<float> position) const
{
    int closestBand = -1;
    float closestDistance = 100000.0f;

    for (int band = 0; band < 6; ++band)
    {
        const float frequency =
            processor.parameters.getRawParameterValue(
                parameterID("FREQ", band))->load();

        const float gain =
            processor.parameters.getRawParameterValue(
                parameterID("GAIN", band))->load();

        const float x = frequencyToX(frequency);
        const float y = gainToY(gain);

        const float distance =
            position.getDistanceFrom({ x, y });

        if (distance < closestDistance)
        {
            closestDistance = distance;
            closestBand = band;
        }
    }

    return closestDistance < 28.0f ? closestBand : -1;
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::updateBandFromMouse(
    int band,
    juce::Point<float> position)
{
    if (band < 0 || band >= 6)
        return;

    const float frequency = xToFrequency(position.x);
    const float gain = yToGain(position.y);

    setParameter(
        processor.parameters,
        parameterID("FREQ", band),
        frequency);

    setParameter(
        processor.parameters,
        parameterID("GAIN", band),
        gain);

    editor.updateSelectedBand();
    repaint();
}

//==============================================================================
// Graph Paint
//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::paint(
    juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    g.fillAll(panelColour);

    g.setColour(juce::Colour(38, 46, 55));
    g.drawRoundedRectangle(bounds.reduced(0.5f), 7.0f, 1.0f);

    const float width = bounds.getWidth();
    const float height = bounds.getHeight();

    // Horizontal grid.
    for (int i = 0; i <= 8; ++i)
    {
        const float y = height * static_cast<float>(i) / 8.0f;

        g.setColour(i == 4
            ? juce::Colour(90, 105, 115)
            : juce::Colour(48, 57, 67));

        g.drawHorizontalLine(
            juce::roundToInt(y), 0.0f, width);
    }

    // Vertical frequency grid.
    const std::array<float, 10> frequencies
    {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    for (const float frequency : frequencies)
    {
        const float x = frequencyToX(frequency);

        g.setColour(juce::Colour(48, 57, 67));
        g.drawVerticalLine(
            juce::roundToInt(x), 0.0f, height);

        g.setColour(mutedText);
        g.setFont(10.0f);

        juce::String label;

        if (frequency >= 1000.0f)
            label = juce::String(frequency / 1000.0f, 1) + "k";
        else
            label = juce::String(juce::roundToInt(frequency));

        g.drawText(
            label,
            juce::roundToInt(x) - 19,
            juce::roundToInt(height - 17.0f),
            38,
            13,
            juce::Justification::centred);
    }

    // Gain scale.
    for (int gain = -18; gain <= 18; gain += 6)
    {
        const float y = gainToY(static_cast<float>(gain));

        g.setColour(mutedText);
        g.setFont(10.0f);

        g.drawText(
            juce::String(gain),
            4,
            juce::roundToInt(y - 7.0f),
            30,
            14,
            juce::Justification::left);
    }

    // FFT spectrum.
    juce::Path spectrumPath;

    const double sampleRate = processor.getSampleRate();

    if (sampleRate > 0.0)
    {
        bool started = false;

        for (int x = 0; x < getWidth(); ++x)
        {
            const float frequency = xToFrequency(
                static_cast<float>(x));

            const int fftIndex = juce::roundToInt(
                frequency * 1024.0 / sampleRate);

            if (fftIndex < 1 || fftIndex >= 512)
                continue;

            const float magnitude =
                processor.getSpectrumValue(fftIndex);

            const float y = height - 24.0f
                - magnitude * (height * 0.65f);

            if (!started)
            {
                spectrumPath.startNewSubPath(
                    static_cast<float>(x), y);

                started = true;
            }
            else
            {
                spectrumPath.lineTo(
                    static_cast<float>(x), y);
            }
        }
    }

    g.setColour(turquoise.withAlpha(0.75f));
    g.strokePath(
        spectrumPath,
        juce::PathStrokeType(1.4f));

    // EQ response curve.
    juce::Path responsePath;

    bool responseStarted = false;

    for (int x = 0; x < getWidth(); ++x)
    {
        const float frequency =
            xToFrequency(static_cast<float>(x));

        float totalGain = 0.0f;

        for (int band = 0; band < 6; ++band)
        {
            const bool enabled =
                processor.parameters.getRawParameterValue(
                    parameterID("BANDON", band))->load() > 0.5f;

            const int type = juce::roundToInt(
                processor.parameters.getRawParameterValue(
                    parameterID("TYPE", band))->load());

            if (!enabled || type == 0)
                continue;

            const float center =
                processor.parameters.getRawParameterValue(
                    parameterID("FREQ", band))->load();

            const float gain =
                processor.parameters.getRawParameterValue(
                    parameterID("GAIN", band))->load();

            const float q =
                processor.parameters.getRawParameterValue(
                    parameterID("Q", band))->load();

            const float ratio = std::log2(
                juce::jmax(1.0f, frequency)
                / juce::jmax(1.0f, center));

            const float widthValue =
                juce::jmax(0.15f, 1.0f / q);

            const float bell =
                std::exp(-0.5f * std::pow(
                    ratio / widthValue, 2.0f));

            switch (type)
            {
                case 1: // Low pass
                    totalGain -= 18.0f /
                        (1.0f + std::exp(
                            -ratio * 5.0f / widthValue));
                    break;

                case 2: // Band pass
                    totalGain += 12.0f * bell - 12.0f;
                    break;

                case 3: // High pass
                    totalGain -= 18.0f /
                        (1.0f + std::exp(
                            ratio * 5.0f / widthValue));
                    break;

                case 4: // Notch
                    totalGain -= 18.0f * bell;
                    break;

                case 5: // Low shelf
                    totalGain += gain /
                        (1.0f + std::exp(
                            ratio * 5.0f / widthValue));
                    break;

                case 6: // Peaking
                    totalGain += gain * bell;
                    break;

                case 7: // High shelf
                    totalGain += gain /
                        (1.0f + std::exp(
                            -ratio * 5.0f / widthValue));
                    break;

                default:
                    break;
            }
        }

        totalGain = juce::jlimit(-18.0f, 18.0f, totalGain);

        const float y = gainToY(totalGain);

        if (!responseStarted)
        {
            responsePath.startNewSubPath(
                static_cast<float>(x), y);

            responseStarted = true;
        }
        else
        {
            responsePath.lineTo(
                static_cast<float>(x), y);
        }
    }

    // Fill under response curve.
    juce::Path fillPath = responsePath;

    if (responseStarted)
    {
        fillPath.lineTo(width, gainToY(0.0f));
        fillPath.lineTo(0.0f, gainToY(0.0f));
        fillPath.closeSubPath();

        g.setColour(yellow.withAlpha(0.07f));
        g.fillPath(fillPath);
    }

    g.setColour(yellow);
    g.strokePath(
        responsePath,
        juce::PathStrokeType(
            2.2f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Band nodes.
    for (int band = 0; band < 6; ++band)
    {
        const bool enabled =
            processor.parameters.getRawParameterValue(
                parameterID("BANDON", band))->load() > 0.5f;

        const int type = juce::roundToInt(
            processor.parameters.getRawParameterValue(
                parameterID("TYPE", band))->load());

        if (!enabled || type == 0)
            continue;

        const float frequency =
            processor.parameters.getRawParameterValue(
                parameterID("FREQ", band))->load();

        const float gain =
            processor.parameters.getRawParameterValue(
                parameterID("GAIN", band))->load();

        const float x = frequencyToX(frequency);
        const float y = gainToY(gain);

        const bool selected = band == editor.selectedBand;

        const float radius = selected ? 8.0f : 6.0f;

        g.setColour(
            bandColours[static_cast<size_t>(band)].withAlpha(0.22f));

        g.fillEllipse(
            x - radius - 5.0f,
            y - radius - 5.0f,
            (radius + 5.0f) * 2.0f,
            (radius + 5.0f) * 2.0f);

        g.setColour(bandColours[static_cast<size_t>(band)]);

        g.fillEllipse(
            x - radius,
            y - radius,
            radius * 2.0f,
            radius * 2.0f);

        g.setColour(juce::Colours::white);
        g.drawEllipse(
            x - radius,
            y - radius,
            radius * 2.0f,
            radius * 2.0f,
            selected ? 2.0f : 1.0f);

        g.setFont(10.0f);
        g.drawText(
            juce::String(band + 1),
            juce::Rectangle<float>(
                x - 10.0f, y - 25.0f, 20.0f, 14.0f),
            juce::Justification::centred);
    }
}

//==============================================================================
// Graph Mouse Controls
//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDown(
    const juce::MouseEvent& event)
{
    mouseDownPosition = event.position;

    draggingBand = findClosestBand(event.position);

    if (draggingBand >= 0)
    {
        editor.selectBand(draggingBand);
        dragStarted = true;

        updateBandFromMouse(
            draggingBand,
            event.position);
    }
    else
    {
        dragStarted = false;
    }
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDrag(
    const juce::MouseEvent& event)
{
    if (dragStarted && draggingBand >= 0)
    {
        updateBandFromMouse(
            draggingBand,
            event.position);
    }
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseUp(
    const juce::MouseEvent&)
{
    draggingBand = -1;
    dragStarted = false;
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseWheelMove(
    const juce::MouseEvent& event,
    const juce::MouseWheelDetails& wheel)
{
    int band = findClosestBand(event.position);

    if (band < 0)
        band = editor.selectedBand;

    if (band < 0 || band >= 6)
        return;

    const float oldGain =
        processor.parameters.getRawParameterValue(
            parameterID("GAIN", band))->load();

    const float newGain = juce::jlimit(
        -18.0f,
        18.0f,
        oldGain + wheel.deltaY * 3.0f);

    setParameter(
        processor.parameters,
        parameterID("GAIN", band),
        newGain);

    editor.selectBand(band);
    repaint();
}

//==============================================================================
// Editor Constructor
//==============================================================================

LaPautaEQProAudioProcessorEditor::LaPautaEQProAudioProcessorEditor(
    LaPautaEQProAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor),
      graph(processor, *this)
{
    setSize(900, 550);
    setResizable(false, false);

    // Title.
    titleLabel.setText(
        "LA PAUTA EQ PRO",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(juce::FontOptions(22.0f).withStyle("Bold")));

    titleLabel.setColour(
        juce::Label::textColourId,
        juce::Colours::white);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "PARAMETRIC EQUALIZER  //  SIX BAND SURGICAL PROCESSING",
        juce::dontSendNotification);

    subtitleLabel.setFont(10.0f);

    subtitleLabel.setColour(
        juce::Label::textColourId,
        mutedText);

    addAndMakeVisible(subtitleLabel);

    // Graph.
    addAndMakeVisible(graph);

    // Band buttons.
    for (int i = 0; i < 6; ++i)
    {
        auto& button = bandButtons[static_cast<size_t>(i)];

        button.setButtonText("BAND " + juce::String(i + 1));

        button.setClickingTogglesState(false);

        button.onClick = [this, i]()
        {
            selectBand(i);
        };

        addAndMakeVisible(button);
    }

    // Sliders.
    styleSlider(frequencySlider);
    styleSlider(gainSlider);
    styleSlider(qSlider);
    styleSlider(outputSlider);

    frequencySlider.setRange(20.0, 20000.0, 1.0);
    frequencySlider.setSkewFactorFromMidPoint(1000.0);
    frequencySlider.setNumDecimalPlacesToDisplay(0);
    frequencySlider.setTextValueSuffix(" Hz");

    gainSlider.setRange(-18.0, 18.0, 0.1);
    gainSlider.setNumDecimalPlacesToDisplay(1);
    gainSlider.setTextValueSuffix(" dB");

    qSlider.setRange(0.1, 10.0, 0.01);
    qSlider.setNumDecimalPlacesToDisplay(2);

    outputSlider.setRange(-24.0, 12.0, 0.1);
    outputSlider.setNumDecimalPlacesToDisplay(1);
    outputSlider.setTextValueSuffix(" dB");

    addAndMakeVisible(frequencySlider);
    addAndMakeVisible(gainSlider);
    addAndMakeVisible(qSlider);
    addAndMakeVisible(outputSlider);

    // Labels.
    const auto setupLabel = [this](juce::Label& label,
                                   const juce::String& text)
    {
        label.setText(text, juce::dontSendNotification);
        label.setFont(juce::Font(12.0f, juce::Font::bold));
        label.setColour(
            juce::Label::textColourId,
            juce::Colours::white);

        label.setJustificationType(
            juce::Justification::centred);

        addAndMakeVisible(label);
    };

    setupLabel(frequencyLabel, "FREQUENCY");
    setupLabel(gainLabel, "GAIN");
    setupLabel(qLabel, "Q");
    setupLabel(outputLabel, "OUTPUT");
    setupLabel(filterTypeLabel, "FILTER TYPE");

    // Filter selector.
    filterTypeBox.addItem("Disabled", 1);
    filterTypeBox.addItem("Low pass", 2);
    filterTypeBox.addItem("Band pass", 3);
    filterTypeBox.addItem("High pass", 4);
    filterTypeBox.addItem("Band stop", 5);
    filterTypeBox.addItem("Low shelf", 6);
    filterTypeBox.addItem("Peaking", 7);
    filterTypeBox.addItem("High shelf", 8);

    filterTypeBox.setColour(
        juce::ComboBox::backgroundColourId,
        panelColour);

    filterTypeBox.setColour(
        juce::ComboBox::textColourId,
        juce::Colours::white);

    filterTypeBox.setColour(
        juce::ComboBox::outlineColourId,
        turquoise);

    addAndMakeVisible(filterTypeBox);

    // Output attachment.
    outputAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "OUTPUT",
                outputSlider);

    selectBand(0);

    startTimerHz(30);
}

//==============================================================================
// Destructor
//==============================================================================

LaPautaEQProAudioProcessorEditor::~LaPautaEQProAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
// Slider Styling
//==============================================================================

void LaPautaEQProAudioProcessorEditor::styleSlider(
    juce::Slider& slider)
{
    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        85,
        20);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        turquoise);

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        juce::Colour(55
