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
    params.push_back(std::make_unique<juce::AudioParameterFloat>("LOWGAIN", "Low Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("MIDGAIN", "Mid Gain", -24.0f, 24.0f, 0.0f));
    params.push_back(std::make_unique<juce::AudioParameterFloat>("HIGHGAIN", "High Gain", -24.0f, 24.0f, 0.0f));
    return { params.begin(), params.end() };
}
const juce::String LaPautaEQProAudioProcessor::getName() const { return "LaPautaEQPro"; }
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
    spec.numChannels = (juce::uint32)getTotalNumOutputChannels();
    filterChain.prepare(spec);
}
void LaPautaEQProAudioProcessor::releaseResources() {}
bool LaPautaEQProAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}
void LaPautaEQProAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());
    auto lowGain = apvts.getRawParameterValue("LOWGAIN")->load();
    auto midGain = apvts.getRawParameterValue("MIDGAIN")->load();
    auto highGain = apvts.getRawParameterValue("HIGHGAIN")->load();
    auto* lowCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 100.0, 1.0, juce::Decibels::decibelsToGain(lowGain));
    auto* midCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 1000.0, 1.0, juce::Decibels::decibelsToGain(midGain));
    auto* highCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), 8000.0, 1.0, juce::Decibels::decibelsToGain(highGain));
    *filterChain.get<0>().coefficients = *lowCoeffs;
    *filterChain.get<1>().coefficients = *midCoeffs;
    *filterChain.get<2>().coefficients = *highCoeffs;
    juce::dsp::AudioBlock<float> block(buffer);
    juce::dsp::ProcessContextReplacing<float> context(block);
    filterChain.process(context);
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
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LaPautaEQProAudioProcessor(); }
