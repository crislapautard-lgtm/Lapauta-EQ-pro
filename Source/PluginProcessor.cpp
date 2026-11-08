#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaEQProAudioProcessor::LaPautaEQProAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true).withOutput ("Output", juce::AudioChannelSet::stereo(), true))
{
}

LaPautaEQProAudioProcessor::~LaPautaEQProAudioProcessor() {}

const juce::String LaPautaEQProAudioProcessor::getName() const { return "La Pauta EQ Pro"; }
bool LaPautaEQProAudioProcessor::acceptsMidi() const { return false; }
bool LaPautaEQProAudioProcessor::producesMidi() const { return false; }
bool LaPautaEQProAudioProcessor::isMidiEffect() const { return false; }
double LaPautaEQProAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int LaPautaEQProAudioProcessor::getNumPrograms() { return 1; }
int LaPautaEQProAudioProcessor::getCurrentProgram() { return 0; }
void LaPautaEQProAudioProcessor::setCurrentProgram (int) {}
const juce::String LaPautaEQProAudioProcessor::getProgramName (int) { return {}; }
void LaPautaEQProAudioProcessor::changeProgramName (int, const juce::String&) {}

juce::AudioProcessorValueTreeState::ParameterLayout LaPautaEQProAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    const float freqs[8] = { 60.0f, 120.0f, 250.0f, 500.0f, 1000.0f, 2000.0f, 4000.0f, 8000.0f };
    for (int i = 0; i < 8; ++i)
    {
        auto id = "BAND" + juce::String (i + 1);
        layout.add (std::make_unique<juce::AudioParameterFloat> (id + "_FREQ", id + " Freq", juce::NormalisableRange<float> (20.0f, 20000.0f, 1.0f, 0.3f), freqs[i]));
        layout.add (std::make_unique<juce::AudioParameterFloat> (id + "_GAIN", id + " Gain", juce::NormalisableRange<float> (-15.0f, 15.0f, 0.1f), 0.0f));
        layout.add (std::make_unique<juce::AudioParameterFloat> (id + "_Q", id + " Q", juce::NormalisableRange<float> (0.3f, 10.0f, 0.1f), 1.0f));
        layout.add (std::make_unique<juce::AudioParameterBool> (id + "_ON", id + " On", true));
    }
    layout.add (std::make_unique<juce::AudioParameterBool> ("BYPASS", "Bypass", false));
    layout.add (std::make_unique<juce::AudioParameterChoice> ("MODE", "Mode", juce::StringArray ({ "Stereo", "Mid", "Side" }), 0));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("OUTPUT", "Output", juce::NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("MIX", "Mix", juce::NormalisableRange<float> (0.0f, 100.0f, 1.0f), 100.0f));
    return layout;
}

void LaPautaEQProAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::dsp::ProcessSpec spec;
    spec.sampleRate = sampleRate;
    spec.maximumBlockSize = (juce::uint32) samplesPerBlock;
    spec.numChannels = 2;
    leftChain.prepare (spec);
    rightChain.prepare (spec);
    updateFilters();
}

void LaPautaEQProAudioProcessor::releaseResources() {}

bool LaPautaEQProAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
        && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void LaPautaEQProAudioProcessor::updateFilters()
{
    auto sampleRate = getSampleRate();
    float freqs[8], gains[8], qs[8];
    for (int i = 0; i < 8; ++i)
    {
        auto id = "BAND" + juce::String (i + 1);
        freqs[i] = *apvts.getRawParameterValue (id + "_FREQ");
        gains[i] = *apvts.getRawParameterValue (id + "_GAIN");
        qs[i] = *apvts.getRawParameterValue (id + "_Q");
    }
    for (int i = 0; i < 8; ++i)
    {
        auto makeCoeff = [&](int idx, float f, float g, float q)
        {
            if (idx == 0) return *Coefficients::makeLowShelf (sampleRate, f, q, juce::Decibels::decibelsToGain (g));
            if (idx == 7) return *Coefficients::makeHighShelf (sampleRate, f, q, juce::Decibels::decibelsToGain (g));
            return *Coefficients::makePeakFilter (sampleRate, f, q, juce::Decibels::decibelsToGain (g));
        };
        // Asignar a cada banda de la chain
        if (i == 0) { *leftChain.get<0>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<0>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 1) { *leftChain.get<1>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<1>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 2) { *leftChain.get<2>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<2>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 3) { *leftChain.get<3>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<3>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 4) { *leftChain.get<4>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<4>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 5) { *leftChain.get<5>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<5>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 6) { *leftChain.get<6>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<6>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
        if (i == 7) { *leftChain.get<7>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); *rightChain.get<7>().coefficients = makeCoeff (i, freqs[i], gains[i], qs[i]); }
    }
}

void LaPautaEQProAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    bool bypass = *apvts.getRawParameterValue ("BYPASS") > 0.5f;
    updateFilters();

    if (! bypass && buffer.getNumChannels() >= 2)
    {
        juce::dsp::AudioBlock<float> block (buffer);
        auto leftBlock = block.getSingleChannelBlock (0);
        auto rightBlock = block.getSingleChannelBlock (1);
        juce::dsp::ProcessContextReplacing<float> leftCtx (leftBlock);
        juce::dsp::ProcessContextReplacing<float> rightCtx (rightBlock);
        leftChain.process (leftCtx);
        rightChain.process (rightCtx);
    }

    if (buffer.getNumSamples() > 0)
        analyserValue = buffer.getMagnitude (0, 0, buffer.getNumSamples());

    float outGain = juce::Decibels::decibelsToGain ((float) *apvts.getRawParameterValue ("OUTPUT"));
    buffer.applyGain (outGain);
}

bool LaPautaEQProAudioProcessor::hasEditor() const { return true; }
juce::AudioProcessorEditor* LaPautaEQProAudioProcessor::createEditor() { return new LaPautaEQProAudioProcessorEditor (*this); }
void LaPautaEQProAudioProcessor::getStateInformation (juce::MemoryBlock& destData) { auto state = apvts.copyState(); std::unique_ptr<juce::XmlElement> xml (state.createXml()); copyXmlToBinary (*xml, destData); }
void LaPautaEQProAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes)); if (xmlState.get() != nullptr) if (xmlState->hasTagName (apvts.state.getType())) apvts.replaceState (juce::ValueTree::fromXml (*xmlState)); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new LaPautaEQProAudioProcessor(); }
