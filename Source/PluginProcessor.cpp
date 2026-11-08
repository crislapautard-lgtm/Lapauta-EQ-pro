
#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    constexpr std::array<float, 6> defaultFrequencies {
        80.0f, 200.0f, 500.0f, 1500.0f, 5000.0f, 12000.0f
    };

    juce::String frequencyID(int band)
    {
        return "FREQ" + juce::String(band + 1);
    }

    juce::String gainID(int band)
    {
        return "GAIN" + juce::String(band + 1);
    }

    juce::String qID(int band)
    {
        return "Q" + juce::String(band + 1);
    }
}

LaPautaEQProAudioProcessor::LaPautaEQProAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
LaPautaEQProAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    for (int i = 0; i < numBands; ++i)
    {
        const auto suffix = juce::String(i + 1);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            frequencyID(i), "Frequency " + suffix,
            juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.25f),
            defaultFrequencies[static_cast<size_t>(i)]));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            gainID(i), "Gain " + suffix,
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f),
            0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            qID(i), "Q " + suffix,
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f, 0.5f),
            1.0f));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT", "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f));

    return layout;
}

const juce::String LaPautaEQProAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LaPautaEQProAudioProcessor::prepareToPlay(double sampleRate, int)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = 8192;
    spec.numChannels = 1;

    for (auto& band : filters)
    {
        for (auto& filter : band)
        {
            filter.reset();
            filter.prepare(spec);
        }
    }

    updateFilters();
    fifoIndex.store(0);
    fifo.fill(0.0f);
    spectrum.fill(0.0f);
}

void LaPautaEQProAudioProcessor::releaseResources()
{
}

void LaPautaEQProAudioProcessor::updateFilters()
{
    const double sampleRate = getSampleRate();

    if (sampleRate <= 0.0)
        return;

    for (int band = 0; band < numBands; ++band)
    {
        const float frequency = parameters.getRawParameterValue(frequencyID(band))->load();
        const float gain = parameters.getRawParameterValue(gainID(band))->load();
        const float q = parameters.getRawParameterValue(qID(band))->load();

        auto coeff = Coefficients::makePeakFilter(
            sampleRate,
            juce::jlimit(20.0f, static_cast<float>(sampleRate * 0.45), frequency),
            q,
            juce::Decibels::decibelsToGain(gain));

        for (int channel = 0; channel < 2; ++channel)
            filters[static_cast<size_t>(band)][static_cast<size_t>(channel)].coefficients = coeff;
    }
}

void LaPautaEQProAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    for (int band = 0; band < numBands; ++band)
    {
        const float frequency = parameters.getRawParameterValue(frequencyID(band))->load();
        const float gain = parameters.getRawParameterValue(gainID(band))->load();
        const float q = parameters.getRawParameterValue(qID(band))->load();

        auto coeff = Coefficients::makePeakFilter(
            getSampleRate(),
            juce::jlimit(20.0f, static_cast<float>(getSampleRate() * 0.45), frequency),
            q,
            juce::Decibels::decibelsToGain(gain));

        for (int channel = 0; channel < channels; ++channel)
        {
            filters[static_cast<size_t>(band)][static_cast<size_t>(channel)].coefficients = coeff;
            filters[static_cast<size_t>(band)][static_cast<size_t>(channel)].process(
                juce::dsp::ProcessContextReplacing<float>(
                    juce::dsp::AudioBlock<float>(
                        buffer.getArrayOfWritePointers() + channel, 1,
                        static_cast<size_t>(samples))));
        }
    }

    const float outputDb = parameters.getRawParameterValue("OUTPUT")->load();
    buffer.applyGain(juce::Decibels::decibelsToGain(outputDb));

    if (channels > 0)
    {
        for (int i = 0; i < samples; ++i)
        {
            const float mono = buffer.getNumChannels() > 1
                ? 0.5f * (buffer.getSample(0, i) + buffer.getSample(1, i))
                : buffer.getSample(0, i);

            const int index = fifoIndex.fetch_add(1) % fftSize;
            fifo[static_cast<size_t>(index)] = mono;

            if (index == fftSize - 1)
            {
                for (int j = 0; j < fftSize; ++j)
                    fftData[static_cast<size_t>(j)] = fifo[static_cast<size_t>(j)];

                std::fill(fftData.begin() + fftSize, fftData.end(), 0.0f);
                window.multiplyWithWindowingTable(fftData.data(), fftSize);
                forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

                for (int j = 0; j < fftSize / 2; ++j)
                    spectrum[static_cast<size_t>(j)] = fftData[static_cast<size_t>(j)];
            }
        }
    }
}

float LaPautaEQProAudioProcessor::getSpectrumValue(int index) const
{
    if (index < 0 || index >= fftSize / 2)
        return 0.0f;

    return spectrum[static_cast<size_t>(index)];
}

void LaPautaEQProAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LaPautaEQProAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor* LaPautaEQProAudioProcessor::createEditor()
{
    return new LaPautaEQProAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaEQProAudioProcessor();
}
