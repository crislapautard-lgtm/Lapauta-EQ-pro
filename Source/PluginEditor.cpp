#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaEQProAudioProcessorEditor::LaPautaEQProAudioProcessorEditor(LaPautaEQProAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setSize(600, 300);
    lowSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    midSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    highSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    lowLabel.setText("LOW", juce::dontSendNotification);
    midLabel.setText("MID", juce::dontSendNotification);
    highLabel.setText("HIGH", juce::dontSendNotification);
    lowLabel.setJustificationType(juce::Justification::centred);
    midLabel.setJustificationType(juce::Justification::centred);
    highLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(lowSlider);
    addAndMakeVisible(midSlider);
    addAndMakeVisible(highSlider);
    addAndMakeVisible(lowLabel);
    addAndMakeVisible(midLabel);
    addAndMakeVisible(highLabel);
    lowAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "LOWGAIN", lowSlider);
    midAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "MIDGAIN", midSlider);
    highAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, "HIGHGAIN", highSlider);
}

LaPautaEQProAudioProcessorEditor::~LaPautaEQProAudioProcessorEditor() {}

void LaPautaEQProAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::black);
    g.setColour(juce::Colours::white);
    g.setFont(20.0f);
    g.drawFittedText("LaPauta EQ Pro", getLocalBounds().removeFromTop(40), juce::Justification::centred, 1);
}

void LaPautaEQProAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    bounds.removeFromTop(40);
    auto labelRow = bounds.removeFromBottom(30);
    int w = bounds.getWidth() / 3;
    lowSlider.setBounds(bounds.removeFromLeft(w).reduced(20));
    midSlider.setBounds(bounds.removeFromLeft(w).reduced(20));
    highSlider.setBounds(bounds.reduced(20));
    lowLabel.setBounds(labelRow.removeFromLeft(w));
    midLabel.setBounds(labelRow.removeFromLeft(w));
    highLabel.setBounds(labelRow);
}
