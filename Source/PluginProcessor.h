#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class LaPautaEQProAudioProcessor final : public juce::AudioProcessor
{
public:
    LaPautaEQProAudioProcessor();
    ~LaPautaEQProAudioProcessor() override = default;

    static constexpr int numberOfBands = 8;
    static constexpr int analyzerSize = 2048;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(
        juce::AudioBuffer<float>& buffer,
        juce::MidiBuffer& midiMessages) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;

    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;

    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;

    void setStateInformation(
        const void* data,
        int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    static juce::AudioProcessorValueTreeState::ParameterLayout
        createParameterLayout();

    void getAnalyzerData(float* destination, int numSamples) const;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    std::array<std::array<Filter, numberOfBands>, 2> filters;

    std::array<std::atomic<float>, analyzerSize> analyzerBuffer;
    std::atomic<int> analyzerWritePosition { 0 };

    juce::AudioBuffer<float> dryBuffer;

    void updateFilters();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(
        LaPautaEQProAudioProcessor)
};
