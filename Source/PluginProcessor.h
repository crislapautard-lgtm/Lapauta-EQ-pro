#pragma once
#include <JuceHeader.h>

class LaPautaEQProAudioProcessor : public juce::AudioProcessor
{
public:
    LaPautaEQProAudioProcessor();
    ~LaPautaEQProAudioProcessor() override;
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
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
    void setStateInformation(const void* data, int sizeInBytes) override;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameters();
    juce::AudioProcessorValueTreeState apvts;
    static constexpr int fftOrder = 11;
    static constexpr int fftSize = 1 << fftOrder;
    float fftData[2 * fftSize] = {};
    bool nextFFTBlockReady = false;
private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using MonoChain = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter, Filter, Filter>;
    MonoChain leftChain, rightChain;
    void updateFilters();
    float fifo[fftSize] = {};
    int fifoIndex = 0;
    void pushSample(float s);
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPautaEQProAudioProcessor)
};
