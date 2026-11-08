#include "PluginProcessor.h"
#include "PluginEditor.h"

LaPautaEQProAudioProcessorEditor::LaPautaEQProAudioProcessorEditor(LaPautaEQProAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), fft(p.fftOrder), window(p.fftSize, juce::dsp::WindowingFunction<float>::hann)
{
    setSize(900, 560);
    scopeData.resize(p.fftSize / 2, -100.0f);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("LA PAUTA EQ PRO", juce::dontSendNotification);
    titleLabel.setColour(juce::Label::textColourId, juce::Colours::white);
    titleLabel.setFont(juce::Font(24.0f, juce::Font::bold));
    addAndMakeVisible(bandLabel);
    bandLabel.setColour(juce::Label::textColourId, juce::Colours::yellow);
    for (int i = 0; i < 6; ++i)
    {
        addAndMakeVisible(bandButtons[i]);
        bandButtons[i].setButtonText("Banda " + juce::String(i + 1));
        bandButtons[i].setColour(juce::TextButton::buttonColourId, i == 0 ? juce::Colours::orange : juce::Colour(0xff2a2f36));
        bandButtons[i].onClick = [this, i] {
            selectedBand = i; updateAttachments();
            for (int j = 0; j < 6; ++j) bandButtons[j].setColour(juce::TextButton::buttonColourId, j == i ? juce::Colours::orange : juce::Colour(0xff2a2f36));
        };
    }
    addAndMakeVisible(freqSlider); addAndMakeVisible(gainSlider); addAndMakeVisible(qSlider); addAndMakeVisible(outSlider);
    addAndMakeVisible(freqLabel); addAndMakeVisible(gainLabel); addAndMakeVisible(qLabel); addAndMakeVisible(outLabel);
    freqLabel.setText("FREQ", juce::dontSendNotification); gainLabel.setText("GAIN", juce::dontSendNotification); qLabel.setText("Q", juce::dontSendNotification); outLabel.setText("OUTPUT", juce::dontSendNotification);
    for (auto* l : { &freqLabel, &gainLabel, &qLabel, &outLabel }) l->setColour(juce::Label::textColourId, juce::Colours::white);
    for (auto* s : { &freqSlider, &gainSlider, &qSlider, &outSlider }) { s->setSliderStyle(juce::Slider::RotaryVerticalDrag); s->setTextBoxStyle(juce::Slider::TextBoxBelow, false, 90, 20); }
    updateAttachments();
    startTimerHz(30);
}
LaPautaEQProAudioProcessorEditor::~LaPautaEQProAudioProcessorEditor() { stopTimer(); }
void LaPautaEQProAudioProcessorEditor::updateAttachments()
{
    juce::String id = "BAND" + juce::String(selectedBand);
    bandLabel.setText("EDITANDO BANDA " + juce::String(selectedBand + 1) + "  -  Arrastra los puntos en la pantalla", juce::dontSendNotification);
    freqAttach.reset(); gainAttach.reset(); qAttach.reset();
    freqAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id + "_FREQ", freqSlider);
    gainAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id + "_GAIN", gainSlider);
    qAttach = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(audioProcessor.apvts, id + "_Q", qSlider);
}
void LaPautaEQProAudioProcessorEditor::timerCallback()
{
    if (audioProcessor.nextFFTBlockReady)
    {
        window.multiplyWithWindowingTable(audioProcessor.fftData, audioProcessor.fftSize);
        fft.performFrequencyOnlyForwardTransform(audioProcessor.fftData);
        for (int i = 0; i < (int)scopeData.size(); ++i)
            scopeData[i] = juce::jlimit(-100.0f, 0.0f, juce::Decibels::gainToDecibels(audioProcessor.fftData[i]));
        audioProcessor.nextFFTBlockReady = false;
    }
    repaint();
}
void LaPautaEQProAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0a0c10));
    auto area = juce::Rectangle<int>(20, 70, 860, 300);
    g.setColour(juce::Colours::white.withAlpha(0.12f));
    for (int i = 0; i < 11; ++i) g.drawHorizontalLine(area.getY() + i * (area.getHeight() / 10), (float)area.getX(), (float)area.getRight());
    g.setColour(juce::Colour(0xff00e5ff));
    juce::Path sp; bool started = false;
    for (int i = 1; i < (int)scopeData.size(); ++i)
    {
        float freq = (float)i / (float)scopeData.size() * 20000.0f;
        float x = area.getX() + (logf(freq / 20.0f) / logf(1000.0f)) * area.getWidth();
        float y = juce::jmap(scopeData[i], -100.0f, 0.0f, (float)area.getBottom(), (float)area.getY());
        if (!started) { sp.startNewSubPath(x, y); started = true; } else sp.lineTo(x, y);
    }
    g.strokePath(sp, juce::PathStrokeType(1.2f));
    drawCurve(g, area);
}
void LaPautaEQProAudioProcessorEditor::drawCurve(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colours::yellow);
    juce::Path p; bool started = false;
    for (int x = 0; x < area.getWidth(); ++x)
    {
        float freq = 20.0f * powf(1000.0f, (float)x / (float)area.getWidth());
        float total = 0;
        for (int b = 0; b < 6; ++b)
        {
            juce::String id = "BAND" + juce::String(b);
            float bf = audioProcessor.apvts.getRawParameterValue(id + "_FREQ")->load();
            float bg = audioProcessor.apvts.getRawParameterValue(id + "_GAIN")->load();
            float bq = audioProcessor.apvts.getRawParameterValue(id + "_Q")->load();
            float dist = logf(freq / bf);
            total += bg * (1.0f / (1.0f + powf(dist * bq * 2.0f, 2.0f)));
        }
        float y = juce::jmap(total, -24.0f, 24.0f, (float)area.getBottom(), (float)area.getY());
        float px = area.getX() + x;
        if (!started) { p.startNewSubPath(px, y); started = true; } else p.lineTo(px, y);
    }
    g.strokePath(p, juce::PathStrokeType(2.5f));
    for (int b = 0; b < 6; ++b)
    {
        juce::String id = "BAND" + juce::String(b);
        float bf = audioProcessor.apvts.getRawParameterValue(id + "_FREQ")->load();
        float bg = audioProcessor.apvts.getRawParameterValue(id + "_GAIN")->load();
        float x = area.getX() + (logf(bf / 20.0f) / logf(1000.0f)) * area.getWidth();
        float y = juce::jmap(bg, -24.0f, 24.0f, (float)area.getBottom(), (float)area.getY());
        g.setColour(b == selectedBand ? juce::Colours::yellow : juce::Colours::white);
        g.fillEllipse(x - 9, y - 9, 18, 18);
        g.setColour(juce::Colours::black);
        g.drawText(juce::String(b + 1), (int)x - 9, (int)y - 9, 18, 18, juce::Justification::centred);
    }
}
void LaPautaEQProAudioProcessorEditor::resized()
{
    titleLabel.setBounds(20, 10, 500, 30);
    bandLabel.setBounds(20, 380, 600, 25);
    for (int i = 0; i < 6; ++i) bandButtons[i].setBounds(20 + i * 110, 410, 100, 30);
    freqLabel.setBounds(100, 450, 80, 20); gainLabel.setBounds(280, 450, 80, 20); qLabel.setBounds(460, 450, 80, 20); outLabel.setBounds(640, 450, 80, 20);
    freqSlider.setBounds(60, 470, 120, 70); gainSlider.setBounds(240, 470, 120, 70); qSlider.setBounds(420, 470, 120, 70); outSlider.setBounds(600, 470, 120, 70);
}
void LaPautaEQProAudioProcessorEditor::mouseDown(const juce::MouseEvent& e)
{
    if (e.y > 70 && e.y < 370 && e.x > 20 && e.x < 880)
    {
        float freq = 20.0f * powf(1000.0f, (float)(e.x - 20) / 860.0f);
        float gain = juce::jmap((float)e.y, 70.0f, 370.0f, 24.0f, -24.0f);
        auto* fp = audioProcessor.apvts.getParameter("BAND" + juce::String(selectedBand) + "_FREQ");
        auto* gp = audioProcessor.apvts.getParameter("BAND" + juce::String(selectedBand) + "_GAIN");
        if (fp != nullptr) fp->setValueNotifyingHost(fp->convertTo0to1(freq));
        if (gp != nullptr) gp->setValueNotifyingHost(gp->convertTo0to1(gain));
    }
}
void LaPautaEQProAudioProcessorEditor::mouseDrag(const juce::MouseEvent& e) { mouseDown(e); }
