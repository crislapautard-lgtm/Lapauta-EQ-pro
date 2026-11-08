
#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <array>

class LaPautaEQProAudioProcessorEditor
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
    class EQGraph : public juce::Component
    {
    public:
        explicit EQGraph(
            LaPautaEQProAudioProcessor& processor,
            LaPautaEQProAudioProcessorEditor& editor);

        void paint(juce::Graphics& g) override;
        void mouseDown(const juce::MouseEvent& event) override;
        void mouseDrag(const juce::MouseEvent& event) override;
        void mouseUp(const juce::MouseEvent& event) override;

        void mouseWheelMove(
            const juce::MouseEvent& event,
            const juce::MouseWheelDetails& wheel) override;

    private:
        LaPautaEQProAudioProcessor& processor;
        LaPautaEQProAudioProcessorEditor& editor;

        int draggingBand = -1;
        bool dragStarted = false;
        juce::Point<float> mouseDownPosition;

        float frequencyToX(float frequency) const;
        float gainToY(float gain) const;
        float xToFrequency(float x) const;
        float yToGain(float y) const;

        int findClosestBand(juce::Point<float> position) const;

        void updateBandFromMouse(
            int band,
            juce::Point<float> position);

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EQGraph)
    };

    LaPautaEQProAudioProcessor& audioProcessor;

    EQGraph graph;

    std::array<juce::TextButton, 6> bandButtons;

    juce::Slider frequencySlider;
    juce::Slider gainSlider;
    juce::Slider qSlider;
    juce::Slider outputSlider;

    juce::ComboBox filterTypeBox;

    juce::Label frequencyLabel;
    juce::Label gainLabel;
    juce::Label qLabel;
    juce::Label outputLabel;
    juce::Label filterTypeLabel;

    juce::Label titleLabel;
    juce::Label subtitleLabel;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        frequencyAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        gainAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        qAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::SliderAttachment>
        outputAttachment;

    std::unique_ptr<
        juce::AudioProcessorValueTreeState::ComboBoxAttachment>
        filterTypeAttachment;

    int selectedBand = 0;

    void timerCallback() override;

    void selectBand(int band);
    void updateSelectedBand();
    void styleSlider(juce::Slider& slider);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        LaPautaEQProAudioProcessorEditor)
};
