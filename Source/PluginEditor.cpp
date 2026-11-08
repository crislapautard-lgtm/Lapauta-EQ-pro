
#include "PluginEditor.h"

namespace
{
    const juce::Colour background(0xff111519);
    const juce::Colour panel(0xff1b2228);
    const juce::Colour turquoise(0xff36d9c5);
    const juce::Colour white(0xffeeeeee);
    const juce::Colour muted(0xff89949b);
}

LaPautaEQProAudioProcessorEditor::
LaPautaEQProAudioProcessorEditor(
    LaPautaEQProAudioProcessor& processor)
    : AudioProcessorEditor(&processor),
      audioProcessor(processor)
{
    setSize(1100, 720);

    titleLabel.setText("LA PAUTA EQ PRO", juce::dontSendNotification);
    titleLabel.setFont(juce::Font(27.0f, juce::Font::bold));
    titleLabel.setColour(juce::Label::textColourId, white);
    addAndMakeVisible(titleLabel);

    subtitleLabel.setText(
        "PROFESSIONAL PARAMETRIC EQUALIZER",
        juce::dontSendNotification);
    subtitleLabel.setFont(juce::Font(11.0f));
    subtitleLabel.setColour(juce::Label::textColourId, turquoise);
    addAndMakeVisible(subtitleLabel);

    outputLabel.setText("OUTPUT", juce::dontSendNotification);
    outputLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(outputLabel);

    mixLabel.setText("DRY / WET", juce::dontSendNotification);
    mixLabel.setColour(juce::Label::textColourId, muted);
    addAndMakeVisible(mixLabel);

    configureSlider(outputSlider);
    configureSlider(mixSlider);

    outputSlider.setRange(-24.0, 12.0, 0.1);
    mixSlider.setRange(0.0, 100.0, 0.1);

    outputAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters, "output", outputSlider);

    mixAttachment =
        std::make_unique<
            juce::AudioProcessorValueTreeState::SliderAttachment>(
                audioProcessor.parameters, "mix", mixSlider);

    addAndMakeVisible(outputSlider);
    addAndMakeVisible(mixSlider);

    analyzerButton.setButtonText("SPECTRUM");
    analyzerButton.setColour(
        juce::TextButton::buttonColourId, panel);
    analyzerButton.setColour(
        juce::TextButton::textColourOffId, turquoise);
    addAndMakeVisible(analyzerButton);

    resetButton.setButtonText("RESET EQ");
    resetButton.setColour(
        juce::TextButton::buttonColourId, panel);
    resetButton.setColour(
        juce::TextButton::textColourOffId, white);

    resetButton.onClick = [this]
    {
        for (int i = 0; i < numberOfBands; ++i)
            gainSliders[i].setValue(
                0.0, juce::sendNotificationSync);
    };

    addAndMakeVisible(resetButton);

    for (int i = 0; i < numberOfBands; ++i)
    {
        const juce::String id = juce::String(i + 1);

        auto& frequency = frequencySliders[i];
        auto& gain = gainSliders[i];
        auto& q = qSliders[i];
        auto& button = bandButtons[i];

        configureSlider(frequency);
        configureSlider(gain);
        configureSlider(q);

        frequency.setRange(20.0, 20000.0, 0.01);
        frequency.setSkewFactorFromMidPoint(1000.0);

        gain.setRange(-18.0, 18.0, 0.01);
        q.setRange(0.1, 10.0, 0.01);

        button.setButtonText("B" + id);
        button.setColour(
            juce::ToggleButton::textColourId, turquoise);

        addAndMakeVisible(button);
        addAndMakeVisible(frequency);
        addAndMakeVisible(gain);
        addAndMakeVisible(q);

        bandAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::ButtonAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_on", button);

        frequencyAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_freq", frequency);

        gainAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_gain", gain);

        qAttachments[i] =
            std::make_unique<
                juce::AudioProcessorValueTreeState::SliderAttachment>(
                    audioProcessor.parameters,
                    "band" + id + "_q", q);
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
    slider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    slider.setTextBoxStyle(
        juce::Slider::TextBoxBelow, false, 70, 20);

    slider.setColour(
        juce::Slider::rotarySliderFillColourId, turquoise);
    slider.setColour(
        juce::Slider::rotarySliderOutlineColourId, panel);
    slider.setColour(
        juce::Slider::thumbColourId, white);
    slider.setColour(
        juce::Slider::textBoxTextColourId, white);
    slider.setColour(
        juce::Slider::textBoxOutlineColourId,
        juce::Colours::transparentBlack);
    slider.setColour(
        juce::Slider::textBoxBackgroundColourId, panel);
}

void LaPautaEQProAudioProcessorEditor::paint(
    juce::Graphics& g)
{
    g.fillAll(background);

    g.setColour(panel);
    g.fillRoundedRectangle(
        18.0f, 105.0f,
        getWidth() - 36.0f, 350.0f,
        12.0f);

    g.setColour(turquoise);
    g.fillRect(18, 90, getWidth() - 36, 2);

    g.setColour(muted);
    g.setFont(12.0f);

    g.drawText(
        "FREQUENCY",
        30, 462, 100, 20,
        juce::Justification::left);

    g.drawText(
        "GAIN (dB)",
        30, 520, 100, 20,
        juce::Justification::left);

    g.drawText(
        "Q",
        30, 635, 100, 20,
        juce::Justification::left);

    g.setColour(turquoise);
    g.setFont(11.0f);

    g.drawText(
        "20 Hz",
        30, 430, 70, 18,
        juce::Justification::left);

    g.drawText(
        "20 kHz",
        getWidth() - 95, 430, 70, 18,
        juce::Justification::right);

    g.setColour(muted);
    g.drawText(
        "LA PAUTA AUDIO TECHNOLOGY",
        25, getHeight() - 25,
        getWidth() - 50, 15,
        juce::Justification::centred);
}

void LaPautaEQProAudioProcessorEditor::resized()
{
    titleLabel.setBounds(28, 18, 350, 40);
    subtitleLabel.setBounds(30, 58, 350, 22);

    analyzerButton.setBounds(700, 28, 130, 34);
    resetButton.setBounds(845, 28, 120, 34);

    outputLabel.setBounds(975, 18, 90, 20);
    outputSlider.setBounds(970, 35, 100, 85);

    mixLabel.setBounds(975, 115, 90, 20);
    mixSlider.setBounds(970, 132, 100, 85);

    const int left = 35;
    const int availableWidth = getWidth() - 70;
    const int columnWidth = availableWidth / numberOfBands;

    for (int i = 0; i < numberOfBands; ++i)
    {
        const int x = left + i * columnWidth;

        bandButtons[i].setBounds(x, 115, columnWidth - 2, 25);
        frequencySliders[i].setBounds(x, 155, columnWidth - 2, 280);
        gainSliders[i].setBounds(x, 480, columnWidth - 2, 100);
        qSliders[i].setBounds(x, 585, columnWidth - 2, 100);
    }
}

void LaPautaEQProAudioProcessorEditor::timerCallback()
{
    repaint();
}

void LaPautaEQProAudioProcessorEditor::updateBandVisibility()
{
    repaint();
}
