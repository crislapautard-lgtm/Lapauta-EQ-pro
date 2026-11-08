
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class LaPautaEQProAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit LaPautaEQProAudioProcessorEditor(
        LaPautaEQProAudioProcessor&);

    ~LaPautaEQProAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    static constexpr int numberOfBands = 24;

    LaPautaEQProAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Slider outputSlider;
    juce::Slider mixSlider;

    juce::Label outputLabel;
    juce::Label mixLabel;

    juce::TextButton analyzerButton;
    juce::TextButton resetButton;

    std::array<juce::Slider, numberOfBands> frequencySliders;
    std::array<juce::Slider, numberOfBands> gainSliders;
    std::array<juce::Slider, numberOfBands> qSliders;

    std::array<juce::ToggleButton, numberOfBands> bandButtons;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> frequencyAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> gainAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> qAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,
        numberOfBands> bandAttachments;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        outputAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAttachment;

    void timerCallback() override;
    void configureSlider(juce::Slider& slider);
    void updateBandVisibility();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        LaPautaEQProAudioProcessorEditor
    )
};
