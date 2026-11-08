#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class LaPautaEQProAudioProcessorEditor : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    LaPautaEQProAudioProcessorEditor(LaPautaEQProAudioProcessor&);
    ~LaPautaEQProAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
    void timerCallback() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
private:
    LaPautaEQProAudioProcessor& audioProcessor;
    juce::dsp::FFT fft;
    juce::dsp::WindowingFunction<float> window;
    std::vector<float> scopeData;
    int selectedBand = 0;
    juce::Slider freqSlider, gainSlider, qSlider, outSlider;
    juce::Label titleLabel, bandLabel, freqLabel, gainLabel, qLabel, outLabel;
    juce::TextButton bandButtons[6];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> freqAttach, gainAttach, qAttach;
    void updateAttachments();
    void drawCurve(juce::Graphics&, juce::Rectangle<int> area);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPautaEQProAudioProcessorEditor)
};
