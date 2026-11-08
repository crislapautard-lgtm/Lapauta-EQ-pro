#include "PluginEditor.h"

namespace
{
    const juce::Colour background(0xff101519);
    const juce::Colour panel(0xff1a2329);
    const juce::Colour panelLight(0xff222d34);
    const juce::Colour turquoise(0xff38d9c6);
    const juce::Colour turquoiseDark(0xff167f79);
    const juce::Colour white(0xffedf4f4);
    const juce::Colour muted(0xff87969d);
    const juce::Colour grid(0xff303b42);
}

LaPautaEQProAudioProcessorEditor::
LaPautaEQProAudioProcessorEditor(
    LaPautaEQProAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(1200, 760);

    titleLabel.setText(
        "LA PAUTA EQ PRO",
        juce::dontSendNotification);

    titleLabel.setFont(
        juce::Font(27.0f, juce::Font::bold));

    titleLabel.setColour(
        juce::Label::textColourId,
        white);

    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "PROFESSIONAL PARAMETRIC EQUALIZER",
        juce::dontSendNotification);

    subtitleLabel.setFont(juce::Font(11.0f));

    subtitleLabel.setColour(
        juce::Label::textColourId,
        turquoise);

    addAndMakeVisible(subtitleLabel);

    outputLabel.setText(
        "OUTPUT",
        juce::dontSendNotification);

    outputLabel.setFont(juce::Font(10.0f));

    outputLabel.setColour(
        juce::Label::textColourId,
        muted);

    addAndMakeVisible(outputLabel);

    mixLabel.setText(
        "DRY / WET",
        juce::dontSendNotification);

    mixLabel.setFont(juce::Font(10.0f));

    mixLabel.setColour(
        juce::Label::textColourId,
        muted);

    addAndMakeVisible(mixLabel);

    configureSlider(outputSlider);
    configureSlider(mixSlider);

    outputSlider.setRange(-24.0, 12.0, 0.1);
    outputSlider.setTextValueSuffix(" dB");

    mixSlider.setRange(0.0, 100.0, 0.1);
    mixSlider.setTextValueSuffix(" %");

    outputAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "output",
                outputSlider);

    mixAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters,
                "mix",
                mixSlider);

    addAndMakeVisible(outputSlider);
    addAndMakeVisible(mixSlider);

    resetButton.setButtonText("RESET EQ");

    resetButton.setColour(
        juce::TextButton::buttonColourId,
        panelLight);

    resetButton.setColour(
        juce::TextButton::textColourOffId,
        turquoise);

    resetButton.onClick = [this]
    {
        for (int i = 0; i < numberOfBands; ++i)
        {
            gainSliders[i].setValue(
                0.0,
                juce::sendNotificationSync);
        }
    };

    addAndMakeVisible(resetButton);

    for (int i = 0; i < numberOfBands; ++i)
    {
        const juce::String id = juce::String(i + 1);

        bandLabels[i].setText(
            "BAND " + id,
            juce::dontSendNotification);

        bandLabels[i].setFont(
            juce::Font(11.0f, juce::Font::bold));

        bandLabels[i].setColour(
            juce::Label::textColourId,
            turquoise);

        bandLabels[i].setJustificationType(
            juce::Justification::centred);

        addAndMakeVisible(bandLabels[i]);

        bandButtons[i].setButtonText("ON");

        bandButtons[i].setColour(
            juce::ToggleButton::textColourId,
            white);

        addAndMakeVisible(bandButtons[i]);

        configureSlider(frequencySliders[i]);
        configureSlider(gainSliders[i]);
        configureSlider(qSliders[i]);

        frequencySliders[i].setRange(
            20.0,
            20000.0,
            0.01);

        frequencySliders[i].setSkewFactorFromMidPoint(
            1000.0);

        frequencySliders[i].setTextValueSuffix(" Hz");

        gainSliders[i].setRange(
            -18.0,
            18.0,
            0.01);

        gainSliders[i].setTextValueSuffix(" dB");

        qSliders[i].setRange(
            0.1,
            10.0,
            0.01);

        qSliders[i].setTextValueSuffix(" Q");

        addAndMakeVisible(frequencySliders[i]);
        addAndMakeVisible(gainSliders[i]);
        addAndMakeVisible(qSliders[i]);

        bandAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_on",
                    bandButtons[i]);

        frequencyAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_freq",
                    frequencySliders[i]);

        gainAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_gain",
                    gainSliders[i]);

        qAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_q",
                    qSliders[i]);
    }

    startTimerHz(30);
}

LaPautaEQProAudioProcessorEditor::
~LaPautaEQProAudioProcessorEditor()
{
    stopTimer();
}

void LaPautaEQProAudioProcessorEditor::configureSlider(
    juce::Slider& slider)
{
    slider.setSliderStyle(
        juce::Slider::RotaryHorizontalVerticalDrag);

    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow,
        false,
        66,
        18);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId,
        turquoise);

    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId,
        panelLight);

    slider.setColour(
        juce::Slider::thumbColourId,
        white);

    slider.setColour(
        juce::Slider::textBoxTextColourId,
        white);

    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);

    slider.setColour(
        juce::Slider::textBoxBackgroundColourId,
        panel);
}

void LaPautaEQProAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(background);

    // Header
    g.setColour(panel);
    g.fillRoundedRectangle(
        18.0f,
        15.0f,
        static_cast<float>(getWidth() - 36),
        68.0f,
        10.0f);

    g.setColour(turquoise);
    g.fillRoundedRectangle(
        18.0f,
        84.0f,
        static_cast<float>(getWidth() - 36),
        2.0f,
        1.0f);

    // Main analyzer panel
    const juce::Rectangle<float> graph(
        24.0f,
        105.0f,
        static_cast<float>(getWidth() - 48),
        315.0f);

    g.setColour(panel);
    g.fillRoundedRectangle(graph, 12.0f);

    g.saveState();
    g.reduceClipRegion(graph.toNearestInt());

    // Grid lines
    g.setColour(grid);
    g.setStrokeType(1.0f);

    for (int i = 1; i < 8; ++i)
    {
        const float x =
            graph.getX() + graph.getWidth() * i / 8.0f;

        g.drawVerticalLine(
            static_cast<int>(x),
            graph.getY() + 25.0f,
            graph.getBottom() - 15.0f);
    }

    for (int i = 1; i < 5; ++i)
    {
        const float y =
            graph.getY() + graph.getHeight() * i / 5.0f;

        g.drawHorizontalLine(
            static_cast<int>(y),
            graph.getX() + 12.0f,
            graph.getRight() - 12.0f);
    }

    // Zero dB reference
    const float zeroY =
        graph.getY() + graph.getHeight() * 0.52f;

    g.setColour(turquoiseDark);
    g.drawHorizontalLine(
        static_cast<int>(zeroY),
        graph.getX() + 10.0f,
        graph.getRight() - 10.0f);

    // Spectrum analyzer
    juce::Path spectrumPath;

    const int spectrumPoints = fftSize / 2;

    for (int i = 1; i < spectrumPoints; ++i)
    {
        const float x =
            graph.getX()
            + (static_cast<float>(i) / spectrumPoints)
            * graph.getWidth();

        const float magnitude = juce::jlimit(
            0.0f,
            1.0f,
            spectrum[static_cast<size_t>(i)] * 4.0f);

        const float y =
            graph.getBottom() - 18.0f - magnitude * 175.0f;

        if (i == 1)
            spectrumPath.startNewSubPath(x, y);
        else
            spectrumPath.lineTo(x, y);
    }

    g.setColour(turquoise.withAlpha(0.85f));
    g.strokePath(
        spectrumPath,
        juce::PathStrokeType(
            2.0f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Bottom labels
    g.setColour(muted);
    g.setFont(10.0f);

    g.drawText(
        "20 Hz",
        static_cast<int>(graph.getX() + 10),
        static_cast<int>(graph.getBottom() - 20),
        55,
        14,
        juce::Justification::left);

    g.drawText(
        "20 kHz",
        static_cast<int>(graph.getRight() - 65),
        static_cast<int>(graph.getBottom() - 20),
        55,
        14,
        juce::Justification::right);

    g.restoreState();

    // Controls panel
    g.setColour(panel);
    g.fillRoundedRectangle(
        18.0f,
        435.0f,
        static_cast<float>(getWidth() - 36),
        295.0f,
        12.0f);

    g.setColour(muted);
    g.setFont(10.0f);

    g.drawText(
        "FREQUENCY",
        25,
        485,
        75,
        16,
        juce::Justification::left);

    g.drawText(
        "GAIN",
        25,
        570,
        75,
        16,
        juce::Justification::left);

    g.drawText(
        "Q",
        25,
        655,
        75,
        16,
        juce::Justification::left);

    g.setColour(turquoise);
    g.setFont(10.0f);

    g.drawText(
        "LA PAUTA AUDIO TECHNOLOGY",
        0,
        getHeight() - 20,
        getWidth(),
        14,
        juce::Justification::centred);
}

void LaPautaEQProAudioProcessorEditor::resized()
{
    titleLabel.setBounds(
        32, 22, 350, 34);

    subtitleLabel.setBounds(
        34, 54, 350, 18);

    resetButton.setBounds(
        getWidth() - 385,
        30,
        100,
        30);

    outputLabel.setBounds(
        getWidth() - 265,
        19,
        85,
        16);

    outputSlider.setBounds(
        getWidth() - 275,
        32,
        100,
        48);

    mixLabel.setBounds(
        getWidth() - 155,
        19,
        85,
        16);

    mixSlider.setBounds(
        getWidth() - 165,
        32,
        100,
        48);

    const int left = 105;
    const int right = getWidth() - 35;
    const int availableWidth = right - left;
    const int columnWidth = availableWidth / numberOfBands;

    for (int i = 0; i < numberOfBands; ++i)
    {
        const int x = left + i * columnWidth;

        bandLabels[i].setBounds(
            x,
            445,
            columnWidth - 5,
            20);

        bandButtons[i].setBounds(
            x + 15,
            465,
            columnWidth - 25,
            22);

        frequencySliders[i].setBounds(
            x + 2,
            490,
            columnWidth - 8,
            75);

        gainSliders[i].setBounds(
            x + 2,
            575,
            columnWidth - 8,
            75);

        qSliders[i].setBounds(
            x + 2,
            660,
            columnWidth - 8,
            65);
    }
}

void LaPautaEQProAudioProcessorEditor::timerCallback()
{
    fftData.fill(0.0f);

    audioProcessor.getAnalyzerData(
        fftData.data(),
        fftSize);

    window.multiplyWithWindowingTable(
        fftData.data(),
        fftSize);

    fft.performFrequencyOnlyForwardTransform(
        fftData.data());

    for (int i = 0; i < fftSize / 2; ++i)
    {
        spectrum[static_cast<size_t>(i)] =
            fftData[static_cast<size_t>(i)];
    }

    repaint();
}
