#pragma once

#include <JuceHeader.h>
#include <array>

#include "PluginProcessor.h"

class LaPautaEQProAudioProcessorEditor final
    : public juce::AudioProcessorEditor,
      private juce::Timer
{
public:
    explicit LaPautaEQProAudioProcessorEditor(
        LaPautaEQProAudioProcessor& processor);

    ~LaPautaEQProAudioProcessorEditor() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    static constexpr int numberOfBands = 8;
    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    LaPautaEQProAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    juce::Label outputLabel;
    juce::Label mixLabel;

    juce::Slider outputSlider;
    juce::Slider mixSlider;

    juce::TextButton resetButton;

    std::array<juce::Label, numberOfBands> bandLabels;
    std::array<juce::ToggleButton, numberOfBands> bandButtons;

    std::array<juce::Slider, numberOfBands> frequencySliders;
    std::array<juce::Slider, numberOfBands> gainSliders;
    std::array<juce::Slider, numberOfBands> qSliders;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment>,
        numberOfBands> bandAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> frequencyAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> gainAttachments;

    std::array<
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,
        numberOfBands> qAttachments;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        outputAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        mixAttachment;

    juce::dsp::FFT fft { fftOrder };

    juce::dsp::WindowingFunction<float> window {
        fftSize,
        juce::dsp::WindowingFunction<float>::hann
    };

    std::array<float, fftSize * 2> fftData {};
    std::array<float, fftSize / 2> spectrum {};

    void timerCallback() override;

    void configureSlider(juce::Slider& slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        LaPautaEQProAudioProcessorEditor)
};
