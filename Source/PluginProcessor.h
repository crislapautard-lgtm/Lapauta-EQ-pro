
#pragma once

#include <JuceHeader.h>
#include <array>
#include <atomic>

class LaPautaEQProAudioProcessor : public juce::AudioProcessor
{
public:
    static constexpr int numBands = 6;

    LaPautaEQProAudioProcessor();
    ~LaPautaEQProAudioProcessor() override = default;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }

    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState parameters;

    float getSpectrumValue(int index) const;

private:
    using Filter = juce::dsp::IIR::Filter<float>;
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    std::array<std::array<Filter, 2>, numBands> filters;
    std::array<std::array<juce::dsp::IIR::Coefficients<float>::Ptr, 2>, numBands> coefficients;

    void updateFilters();

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    static constexpr int fftOrder = 10;
    static constexpr int fftSize = 1 << fftOrder;

    juce::dsp::FFT forwardFFT { fftOrder };
    juce::dsp::WindowingFunction<float> window {
        fftSize,
        juce::dsp::WindowingFunction<float>::hann
    };

    std::array<float, fftSize * 2> fftData {};
    std::atomic<int> fifoIndex { 0 };
    std::array<float, fftSize> fifo {};
    std::array<float, fftSize> spectrum {};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(LaPautaEQProAudioProcessor)
};
