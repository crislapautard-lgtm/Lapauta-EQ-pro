#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class LaPautaEQProAudioProcessorEditor : public juce::AudioProcessorEditor
{
public:
    LaPautaEQProAudioProcessorEditor(LaPautaEQProAudioProcessor&);
    ~LaPautaEQProAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    LaPautaEQProAudioProcessor& audioProcessor;
    juce::Slider lowSlider, midSlider, highSlider;
    juce::Label lowLabel, midLabel, highLabel;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lowAttach, midAttach, highAttach;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPautaEQProAudioProcessorEditor)
};
