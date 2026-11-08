
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// Helpers
//==============================================================================

namespace
{
    using Coefficients = juce::dsp::IIR::Coefficients<float>;

    juce::String bandID(const juce::String& prefix, int band)
    {
        return prefix + juce::String(band + 1);
    }

    constexpr std::array<float, 6> defaultFrequencies
    {
        80.0f, 200.0f, 500.0f, 1500.0f, 5000.0f, 12000.0f
    };

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
            {
                const double omega =
                    juce::MathConstants<double>::twoPi
                    * static_cast<double>(safeFrequency) / sampleRate;

                const double cosine = std::cos(omega);
                const double alpha =
                    std::sin(omega) / (2.0 * static_cast<double>(safeQ));

                return new Coefficients(
                    1.0f,
                    static_cast<float>(-2.0 * cosine),
                    1.0f,
                    static_cast<float>(1.0 + alpha),
                    static_cast<float>(-2.0 * cosine),
                    static_cast<float>(1.0 - alpha));
            }

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

//==============================================================================
// Constructor
//==============================================================================

LaPautaEQProAudioProcessor::LaPautaEQProAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput(
                "Input",
                juce::AudioChannelSet::stereo(),
                true)
            .withOutput(
                "Output",
                juce::AudioChannelSet::stereo(),
                true)),
      parameters(
          *this,
          nullptr,
          "PARAMETERS",
          createParameterLayout())
{
}

//==============================================================================
// Parameter Layout
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout
LaPautaEQProAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    const juce::StringArray filterTypes
    {
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
        const juce::String suffix = juce::String(i + 1);

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            bandID("FREQ", i),
            "Frequency " + suffix,
            juce::NormalisableRange<float>(
                20.0f, 20000.0f, 1.0f, 0.25f),
            defaultFrequencies[static_cast<size_t>(i)]));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            bandID("GAIN", i),
            "Gain " + suffix,
            juce::NormalisableRange<float>(
                -18.0f, 18.0f, 0.1f),
            0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            bandID("Q", i),
            "Q " + suffix,
            juce::NormalisableRange<float>(
                0.1f, 10.0f, 0.01f, 0.5f),
            1.0f));

        layout.add(std::make_unique<juce::AudioParameterChoice>(
            bandID("TYPE", i),
            "Filter Type " + suffix,
            filterTypes,
            6));

        layout.add(std::make_unique<juce::AudioParameterBool>(
            bandID("BANDON", i),
            "Band Enabled " + suffix,
            true));
    }

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "OUTPUT",
        "Output",
        juce::NormalisableRange<float>(
            -24.0f, 12.0f, 0.1f),
        0.0f));

    return layout;
}

//==============================================================================
// Plugin Information
//==============================================================================

const juce::String LaPautaEQProAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

//==============================================================================
// Bus Layout
//==============================================================================

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet()
            == juce::AudioChannelSet::stereo()
        && layouts.getMainOutputChannelSet()
            == juce::AudioChannelSet::stereo();
}

//==============================================================================
// Prepare
//==============================================================================

void LaPautaEQProAudioProcessor::prepareToPlay(
    double sampleRate,
    int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = static_cast<juce::uint32>(
        juce::jmax(1, samplesPerBlock));
    spec.numChannels = 1;

    for (auto& band : filters)
    {
        for (auto& filter : band)
        {
            filter.reset();
            filter.prepare(spec);
        }
    }

    fifoIndex.store(0);
    fifo.fill(0.0f);
    fftData.fill(0.0f);
    spectrum.fill(0.0f);

    updateFilters();
}

//==============================================================================

void LaPautaEQProAudioProcessor::releaseResources()
{
}

//==============================================================================
// Update Filters
//==============================================================================

void LaPautaEQProAudioProcessor::updateFilters()
{
    const double sampleRate = getSampleRate();

    if (sampleRate <= 0.0)
        return;

    for (int band = 0; band < numBands; ++band)
    {
        const float frequency =
            parameters.getRawParameterValue(
                bandID("FREQ", band))->load();

        const float gain =
            parameters.getRawParameterValue(
                bandID("GAIN", band))->load();

        const float q =
            parameters.getRawParameterValue(
                bandID("Q", band))->load();

        const int type = juce::roundToInt(
            parameters.getRawParameterValue(
                bandID("TYPE", band))->load());

        const bool enabled =
            parameters.getRawParameterValue(
                bandID("BANDON", band))->load() > 0.5f;

        Coefficients::Ptr coefficients;

        if (enabled && type != 0)
        {
            coefficients = createBandCoefficients(
                type, sampleRate, frequency, gain, q);
        }

        if (coefficients == nullptr)
        {
            coefficients = Coefficients::makeFirstOrderLowPass(
                sampleRate, static_cast<float>(sampleRate * 0.45));
        }

        for (int channel = 0; channel < 2; ++channel)
        {
            filters[static_cast<size_t>(band)]
                   [static_cast<size_t>(channel)].coefficients = coefficients;
        }
    }
}

//==============================================================================
// Process Audio
//==============================================================================

void LaPautaEQProAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = juce::jmin(2, buffer.getNumChannels());
    const int samples = buffer.getNumSamples();

    if (channels == 0)
        return;

    updateFilters();

    for (int band = 0; band < numBands; ++band)
    {
        const int type = juce::roundToInt(
            parameters.getRawParameterValue(
                bandID("TYPE", band))->load());

        const bool enabled =
            parameters.getRawParameterValue(
                bandID("BANDON", band))->load() > 0.5f;

        if (!enabled || type == 0)
            continue;

        for (int channel = 0; channel < channels; ++channel)
        {
            auto* data = buffer.getWritePointer(channel);

            filters[static_cast<size_t>(band)]
                   [static_cast<size_t>(channel)]
                       .processSamples(data, samples);
        }
    }

    const float outputGain = juce::Decibels::decibelsToGain(
        parameters.getRawParameterValue("OUTPUT")->load());

    buffer.applyGain(outputGain);

    // FFT: captura la señal de salida en mono.
    for (int sample = 0; sample < samples; ++sample)
    {
        float mono = 0.0f;

        for (int channel = 0; channel < channels; ++channel)
            mono += buffer.getSample(channel, sample);

        mono /= static_cast<float>(channels);

        const int index = fifoIndex.load();

        fifo[static_cast<size_t>(index)] = mono;

        const int nextIndex = (index + 1) % fftSize;

        fifoIndex.store(nextIndex);

        if (nextIndex == 0)
        {
            for (int i = 0; i < fftSize; ++i)
                fftData[static_cast<size_t>(i)] =
                    fifo[static_cast<size_t>(i)];

            window.multiplyWithWindowingTable(
                fftData.data(), fftSize);

            forwardFFT.performFrequencyOnlyForwardTransform(
                fftData.data());

            for (int i = 0; i < fftSize / 2; ++i)
            {
                spectrum[static_cast<size_t>(i)] =
                    juce::jlimit(
                        0.0f,
                        1.0f,
                        fftData[static_cast<size_t>(i)] * 0.015f);
            }
        }
    }
}

//==============================================================================
// Spectrum
//==============================================================================

float LaPautaEQProAudioProcessor::getSpectrumValue(int index) const
{
    index = juce::jlimit(0, fftSize / 2 - 1, index);

    return spectrum[static_cast<size_t>(index)];
}

//==============================================================================
// Editor
//==============================================================================

juce::AudioProcessorEditor*
LaPautaEQProAudioProcessor::createEditor()
{
    return new LaPautaEQProAudioProcessorEditor(*this);
}

//==============================================================================
// Save State
//==============================================================================

void LaPautaEQProAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    const auto state = parameters.copyState();

    std::unique_ptr<juce::XmlElement> xml(state.createXml());

    copyXmlToBinary(*xml, destData);
}

//==============================================================================

void LaPautaEQProAudioProcessor::setStateInformation(
    const void* data,
    int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
    {
        parameters.replaceState(
            juce::ValueTree::fromXml(*xml));
    }
}

//==============================================================================
// Plugin Factory
//==============================================================================

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaEQProAudioProcessor();
}
