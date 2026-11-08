
#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaEQProAudioProcessor::LaPautaEQProAudioProcessor()
    : AudioProcessor(
        BusesProperties()
            .withInput("Input", juce::AudioChannelSet::stereo(), true)
            .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout
LaPautaEQProAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "output", "Output",
        juce::NormalisableRange<float>(-24.0f, 12.0f, 0.1f),
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        "mix", "Dry Wet",
        juce::NormalisableRange<float>(0.0f, 100.0f, 0.1f),
        100.0f));

    for (int i = 0; i < numberOfBands; ++i)
    {
        const juce::String id = juce::String(i + 1);

        layout.add(std::make_unique<juce::AudioParameterBool>(
            "band" + id + "_on", "Band " + id + " On", i == 0));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_freq", "Band " + id + " Frequency",
            juce::NormalisableRange<float>(
                20.0f, 20000.0f, 0.01f, 0.25f),
            juce::jlimit(20.0f, 20000.0f,
                40.0f * std::pow(2.0f, i * 0.38f))));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_gain", "Band " + id + " Gain",
            juce::NormalisableRange<float>(-18.0f, 18.0f, 0.01f),
            0.0f));

        layout.add(std::make_unique<juce::AudioParameterFloat>(
            "band" + id + "_q", "Band " + id + " Q",
            juce::NormalisableRange<float>(0.1f, 10.0f, 0.01f),
            1.0f));
    }

    return layout;
}

void LaPautaEQProAudioProcessor::prepareToPlay(
    double sampleRate, int samplesPerBlock)
{
    dryBuffer.setSize(
        getTotalNumOutputChannels(),
        samplesPerBlock);

    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize =
        static_cast<juce::uint32>(samplesPerBlock);
    spec.numChannels = 1;

    for (int i = 0; i < numberOfBands; ++i)
    {
        leftFilters[i].prepare(spec);
        rightFilters[i].prepare(spec);
    }

    updateFilters();
}

void LaPautaEQProAudioProcessor::releaseResources()
{
}

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(
    const BusesLayout& layouts) const
{
    const auto output = layouts.getMainOutputChannelSet();

    return output == juce::AudioChannelSet::mono()
        || output == juce::AudioChannelSet::stereo();
}

void LaPautaEQProAudioProcessor::updateFilters()
{
    const double sampleRate = getSampleRate();

    if (sampleRate <= 0.0)
        return;

    for (int i = 0; i < numberOfBands; ++i)
    {
        const juce::String id = juce::String(i + 1);

        const bool enabled =
            parameters.getRawParameterValue(
                "band" + id + "_on")->load() > 0.5f;

        const float frequency = juce::jlimit(
            20.0f, static_cast<float>(sampleRate * 0.45),
            parameters.getRawParameterValue(
                "band" + id + "_freq")->load());

        const float gain =
            parameters.getRawParameterValue(
                "band" + id + "_gain")->load();

        const float q =
            parameters.getRawParameterValue(
                "band" + id + "_q")->load();

        auto coefficients = Coefficients::makePeakFilter(
            sampleRate,
            frequency,
            q,
            juce::Decibels::decibelsToGain(gain));

        *leftFilters[i].coefficients = *coefficients;
        *rightFilters[i].coefficients = *coefficients;

        leftFilters[i].active = enabled;
        rightFilters[i].active = enabled;
    }
}

void LaPautaEQProAudioProcessor::processBlock(
    juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int channels = buffer.getNumChannels();
    const int samples = buffer.getNumSamples();

    if (dryBuffer.getNumSamples() < samples)
        dryBuffer.setSize(channels, samples, false, false, true);

    dryBuffer.makeCopyOf(buffer, true);

    updateFilters();

    for (int channel = 0; channel < channels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);

        for (int band = 0; band < numberOfBands; ++band)
        {
            auto& filter = channel == 0
                ? leftFilters[band]
                : rightFilters[band];

            if (filter.active)
                filter.process(
                    juce::dsp::AudioBlock<float>(
                        &buffer, channel, 1, samples)
                    .getSingleChannelBlock(0));
        }

        const float mix =
            parameters.getRawParameterValue("mix")->load() * 0.01f;

        const float outputGain = juce::Decibels::decibelsToGain(
            parameters.getRawParameterValue("output")->load());

        for (int sample = 0; sample < samples; ++sample)
        {
            const float dry = dryBuffer.getSample(channel, sample);
            data[sample] =
                (dry * (1.0f - mix) + data[sample] * mix)
                * outputGain;
        }
    }
}

juce::AudioProcessorEditor*
LaPautaEQProAudioProcessor::createEditor()
{
    return new LaPautaEQProAudioProcessorEditor(*this);
}

bool LaPautaEQProAudioProcessor::hasEditor() const
{
    return true;
}

const juce::String LaPautaEQProAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool LaPautaEQProAudioProcessor::acceptsMidi() const { return false; }
bool LaPautaEQProAudioProcessor::producesMidi() const { return false; }
bool LaPautaEQProAudioProcessor::isMidiEffect() const { return false; }

double LaPautaEQProAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int LaPautaEQProAudioProcessor::getNumPrograms() { return 1; }
int LaPautaEQProAudioProcessor::getCurrentProgram() { return 0; }
void LaPautaEQProAudioProcessor::setCurrentProgram(int) {}
const juce::String LaPautaEQProAudioProcessor::getProgramName(int)
{
    return {};
}
void LaPautaEQProAudioProcessor::changeProgramName(
    int, const juce::String&) {}

void LaPautaEQProAudioProcessor::getStateInformation(
    juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void LaPautaEQProAudioProcessor::setStateInformation(
    const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(
        getXmlFromBinary(data, sizeInBytes));

    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(
            juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new LaPautaEQProAudioProcessor();
}
