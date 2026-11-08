
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

    juce::String typeID(int band)
    {
        return "TYPE" + juce::String(band + 1);
    }

    juce::String enabledID(int band)
    {
        return "BANDON" + juce::String(band + 1);
    }

    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    Coefficients::Ptr createBandCoefficients(
        int type,
        double sampleRate,
        float frequency,
        float gain,
        float q)
    {
        if (sampleRate <= 0.0)
            return {};

        const float safeFrequency = juce::jlimit(
            20.0f,
            static_cast<float>(sampleRate * 0.45),
            frequency);

        const float safeQ = juce::jlimit(0.1f, 10.0f, q);

        const float gainFactor =
            juce::Decibels::decibelsToGain(gain);

        switch (type)
        {
            case 1:
                return Coefficients::makeLowPass(
                    sampleRate, safeFrequency, safeQ);

            case 2:
                return Coefficients::makeBandPass(
                    sampleRate, safeFrequency, safeQ);

            case 3:
                return Coefficients::makeHighPass(
                    sampleRate, safeFrequency, safeQ);

            case 4:
                return Coefficients::makeNotchFilter(
                    sampleRate, safeFrequency, safeQ);

            case 5:
                return Coefficients::makeLowShelf(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            case 6:
                return Coefficients::makePeakFilter(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            case 7:
                return Coefficients::makeHighShelf(
                    sampleRate, safeFrequency, safeQ, gainFactor);

            default:
                return {};
        }
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

    const juce::StringArray filterTypes {
        "Disabled",
        "Low pass",
        "Band pass",
        "High pass",
        "Band stop",
        "Low shelf",
        "Peaking",
        "High shelf"
    };

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

        // Peaking is the default to preserve the previous EQ behavior.
        layout.add(std::make_unique<juce::AudioParameterChoice>(
            typeID(i), "Filter Type " + suffix,
            filterTypes, 6));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            enabledID(i), "Band Enabled " + suffix, true));
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

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void LaPautaEQProAudioProcessor::prepareToPlay(
    double sampleRate,
    int)
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
        const float frequency =
            parameters.getRawParameterValue(frequencyID(band))->load();

        const float gain =
            parameters.getRawParameterValue(gainID(band))->load();

        const float q =
            parameters.getRawParameterValue(qID(band))->load();

        const int type = juce::roundToInt(
            parameters.getRawParameterValue(typeID(band))->load());

        const bool enabled =
            parameters.getRawParameterValue(enabledID(band))->load() > 0.5f;

        if (!enabled || type == 0)
            continue;

        auto coeff = createBandCoefficients(
            type, sampleRate, frequency, gain, q);

        if (coeff == nullptr)
            continue;

        for (int channel = 0; channel < 2; ++channel)
        {
            filters[static_cast<size_t>(band)]
                   [static_cast<size_t>(channel)].coefficients = coeff;
        }
    }
}

void LaPautaEQProAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();
    const double sampleRate = getSampleRate();

    if (sampleRate > 0.0)
    {
        for (int band = 0; band < numBands; ++band)
        {
            const float frequency =
                parameters.getRawParameterValue(frequencyID(band))->load();

            const float gain =
                parameters.getRawParameterValue(gainID(band))->load();

            const float q =
                parameters.getRawParameterValue(qID(band))->load();

            const int type = juce::roundToInt(
                parameters.getRawParameterValue(typeID(band))->load());

            const bool enabled =
                parameters.getRawParameterValue(enabledID(band))->load() > 0.5f;

            // Disabled bands do not process the audio.
            if (!enabled || type == 0)
                continue;

            auto coeff = createBandCoefficients(
                type, sampleRate, frequency, gain, q);

            if (coeff == nullptr)
                continue;

            for (int channel = 0; channel < channels; ++channel)
            {
                auto& filter =
                    filters[static_cast<size_t>(band)]
                           [static_cast<size_t>(channel)];

                filter.coefficients = coeff;

                float* channelData = buffer.getWritePointer(channel);

                for (int sample = 0; sample < samples; ++sample)
                    channelData[sample] = filter.processSample(channelData[sample]);
            }
        }
    }

    const float outputDb =
        parameters.getRawParameterValue("OUTPUT")->load();

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

                std::fill(
                    fftData.begin() + fftSize,
                    fftData.end(),
                    0.0f);

                window.multiplyWithWindowingTable(fftData.data(), fftSize);
                forwardFFT.performFrequencyOnlyForwardTransform(fftData.data());

                for (int j = 0; j < fftSize / 2; ++j)
                    spectrum[static_cast<size_t>(j)] =
                        fftData[static_cast<size_t>(j)];
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

void LaPautaEQProAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LaPautaEQProAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessorEditor*
LaPautaEQProAudioProcessor::createEditor()
{
    return new LaPautaEQProAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaEQProAudioProcessor();
}
