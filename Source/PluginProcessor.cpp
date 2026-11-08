#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaEQProAudioProcessor::LaPautaEQProAudioProcessor()
    : AudioProcessor(BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameters())
{
}

LaPautaEQProAudioProcessor::~LaPautaEQProAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout LaPautaEQProAudioProcessor::createParameters()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    float freqs[6] = { 60.0f, 150.0f, 400.0f, 1000.0f, 3500.0f, 10000.0f };
    for (int i = 0; i < 6; ++i)
    {
        juce::String id = "BAND" + juce::String(i);
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id + "_FREQ", id + " Freq", juce::NormalisableRange<float>(20.0f, 20000.0f, 1.0f, 0.3f), freqs[i]));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id + "_GAIN", id + " Gain", -24.0f, 24.0f, 0.0f));
        params.push_back(std::make_unique<juce::AudioParameterFloat>(id + "_Q", id + " Q", 0.1f, 10.0f, 1.0f));
    }
    params.push_back(std::make_unique<juce::AudioParameterFloat>("OUTGAIN", "Output", -12.0f, 12.0f, 0.0f));
    return { params.begin(), params.end() };
}

const juce::String LaPautaEQProAudioProcessor::getName() const { return "LaPauta EQ Pro"; }
bool LaPautaEQProAudioProcessor::acceptsMidi() const { return false; }
bool LaPautaEQProAudioProcessor::producesMidi() const { return false; }
bool LaPautaEQProAudioProcessor::isMidiEffect() const { return false; }
double LaPautaEQProAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int LaPautaEQProAudioProcessor::getNumPrograms() { return 1; }
int LaPautaEQProAudioProcessor::getCurrentProgram() { return 0; }
void LaPautaEQProAudioProcessor::setCurrentProgram(int) {}
const juce::String LaPautaEQProAudioProcessor::getProgramName(int) { return {}; }
void LaPautaEQProAudioProcessor::changeProgramName(int, const juce::String&) {}

void LaPautaEQProAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32)samplesPerBlock;
    spec.numChannels = 1;
    leftChain.prepare(spec);
    rightChain.prepare(spec);
    updateFilters();
}

void LaPautaEQProAudioProcessor::releaseResources() {}

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono() && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo()) return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet()) return false;
    return true;
}

void LaPautaEQProAudioProcessor::updateFilters()
{
    for (int i = 0; i < 6; ++i)
    {
        juce::String id = "BAND" + juce::String(i);
        float freq = apvts.getRawParameterValue(id + "_FREQ")->load();
        float gain = apvts.getRawParameterValue(id + "_GAIN")->load();
        float q = apvts.getRawParameterValue(id + "_Q")->load();
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), freq, q, juce::Decibels::decibelsToGain(gain));
        if (i == 0) { *leftChain.get<0>().coefficients = *coeffs; *rightChain.get<0>().coefficients = *coeffs; }
        if (i == 1) { *leftChain.get<1>().coefficients = *coeffs; *rightChain.get<1>().coefficients = *coeffs; }
        if (i == 2) { *leftChain.get<2>().coefficients = *coeffs; *rightChain.get<2>().coefficients = *coeffs; }
        if (i == 3) { *leftChain.get<3>().coefficients = *coeffs; *rightChain.get<3>().coefficients = *coeffs; }
        if (i == 4) { *leftChain.get<4>().coefficients = *coeffs; *rightChain.get<4>().coefficients = *coeffs; }
        if (i == 5) { *leftChain.get<5>().coefficients = *coeffs; *rightChain.get<5>().coefficients = *coeffs; }
    }
}

void LaPautaEQProAudioProcessor::pushSample(float s)
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            juce::zeromem(fftData, sizeof(fftData));
            memcpy(fftData, fifo, sizeof(fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = s;
}

void LaPautaEQProAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i) buffer.clear(i, 0, buffer.getNumSamples());
    updateFilters();
    for (int s = 0; s < buffer.getNumSamples(); ++s) pushSample(buffer.getSample(0, s));
    juce::dsp::AudioBlock<float> block(buffer);
    auto leftBlock = block.getSingleChannelBlock(0);
    auto rightBlock = block.getSingleChannelBlock(1);
    juce::dsp::ProcessContextReplacing<float> leftCtx(leftBlock);
    juce::dsp::ProcessContextReplacing<float> rightCtx(rightBlock);
    leftChain.process(leftCtx);
    rightChain.process(rightCtx);
    float out = juce::Decibels::decibelsToGain(apvts.getRawParameterValue("OUTGAIN")->load());
    buffer.applyGain(out);
}

bool LaPautaEQProAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LaPautaEQProAudioProcessor::createEditor() { return new LaPautaEQProAudioProcessorEditor(*this); }

void LaPautaEQProAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
void LaPautaEQProAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState.get() != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LaPautaEQProAudioProcessor(); }
