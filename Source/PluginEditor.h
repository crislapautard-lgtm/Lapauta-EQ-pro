#pragma once
#include <JuceHeader.h>
#include <vector>
#include "PluginProcessor.h"

class LaPautaEQProAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    LaPautaEQProAudioProcessorEditor (LaPautaEQProAudioProcessor& p);
    ~LaPautaEQProAudioProcessorEditor() override;
    void paint (juce::Graphics& g) override;
    void resized() override;
    void timerCallback() override;
private:
    LaPautaEQProAudioProcessor& audioProcessor;
    juce::Label titleLabel, subtitleLabel, analyserLabel, outputLabel, mixLabel, modeLabel;
    juce::OwnedArray<juce::Slider> freqSliders, gainSliders, qSliders;
    juce::OwnedArray<juce::Label> bandLabels;
    juce::OwnedArray<juce::ToggleButton> bandOnButtons;

    // CORREGIDO: antes era OwnedArray<unique_ptr<...>> y eso daba error C2665
    // Ahora es std::vector que SI acepta unique_ptr
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>> freqAttachments, gainAttachments, qAttachments;
    std::vector<std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>> onAttachments;

    juce::TextButton bypassButton, resetButton, soloButton, analyserButton;
    juce::ComboBox modeBox;
    juce::Slider outputSlider, mixSlider;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> modeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outputAttachment, mixAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    float analyserLevel = 0.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LaPautaEQProAudioProcessorEditor)
};
