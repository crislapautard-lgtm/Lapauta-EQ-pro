#include "PluginEditor.h"
#include "PluginProcessor.h"

LaPautaEQProAudioProcessorEditor::LaPautaEQProAudioProcessorEditor (LaPautaEQProAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setSize (900, 540);
    startTimerHz (30);

    titleLabel.setText ("LA PAUTA EQ PRO", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (24.0f, juce::Font::bold)));
    titleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff00e5a0));
    titleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("PROFESSIONAL PARAMETRIC EQ  •  8 BANDAS  •  LIVIANO", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (11.0f)));
    subtitleLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a93a6));
    subtitleLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (subtitleLabel);

    analyserLabel.setText ("ANALYSER", juce::dontSendNotification);
    analyserLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold)));
    analyserLabel.setColour (juce::Label::textColourId, juce::Colour (0xff8a93a6));
    addAndMakeVisible (analyserLabel);

    const char* names[8] = { "LOW", "LOW-MID", "MID 1", "MID 2", "MID 3", "HIGH-MID", "HIGH", "AIR" };

    for (int i = 0; i < 8; ++i)
    {
        auto id = "BAND" + juce::String (i + 1);
        auto* f = new juce::Slider(); f->setSliderStyle (juce::Slider::RotaryVerticalDrag); f->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18); f->setRange (20, 20000, 1); f->setSkewFactorFromMidPoint (1000); f->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff00e5a0)); addAndMakeVisible (f); freqSliders.add (f);
        freqAttachments.add (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id + "_FREQ", *f));

        auto* g = new juce::Slider(); g->setSliderStyle (juce::Slider::RotaryVerticalDrag); g->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18); g->setRange (-15, 15, 0.1); g->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xffffffff)); addAndMakeVisible (g); gainSliders.add (g);
        gainAttachments.add (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id + "_GAIN", *g));

        auto* q = new juce::Slider(); q->setSliderStyle (juce::Slider::RotaryVerticalDrag); q->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 78, 18); q->setRange (0.3, 10, 0.1); q->setColour (juce::Slider::rotarySliderFillColourId, juce::Colour (0xff8a93a6)); addAndMakeVisible (q); qSliders.add (q);
        qAttachments.add (std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, id + "_Q", *q));

        auto* lab = new juce::Label(); lab->setText (juce::String (names[i]) + " - B" + juce::String (i+1), juce::dontSendNotification); lab->setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold))); lab->setColour (juce::Label::textColourId, juce::Colour (0xffffffff)); lab->setJustificationType (juce::Justification::centred); addAndMakeVisible (lab); bandLabels.add (lab);

        auto* on = new juce::ToggleButton(); on->setButtonText ("ON"); on->setColour (juce::ToggleButton::textColourId, juce::Colour (0xff00e5a0)); addAndMakeVisible (on); bandOnButtons.add (on);
        onAttachments.add (std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, id + "_ON", *on));
    }

    bypassButton.setButtonText ("BYPASS"); bypassButton.setClickingTogglesState (true); bypassButton.setColour (juce::TextButton::buttonColourId, juce::Colour (0xff202634)); addAndMakeVisible (bypassButton);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (audioProcessor.apvts, "BYPASS", bypassButton);

    resetButton.setButtonText ("RESET"); addAndMakeVisible (resetButton);
    resetButton.onClick = [this] { for (int i=0;i<8;++i){ auto id="BAND"+juce::String(i+1); audioProcessor.apvts.getParameter(id+"_GAIN")->setValueNotifyingHost(0.5f); } };

    soloButton.setButtonText ("SOLO"); addAndMakeVisible (soloButton);
    analyserButton.setButtonText ("ANALYSER ON"); addAndMakeVisible (analyserButton);

    modeLabel.setText ("MODE", juce::dontSendNotification); modeLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold))); addAndMakeVisible (modeLabel);
    modeBox.addItemList ({ "Stereo", "Mid", "Side" }, 1); addAndMakeVisible (modeBox);
    modeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (audioProcessor.apvts, "MODE", modeBox);

    outputLabel.setText ("OUTPUT", juce::dontSendNotification); outputLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold))); addAndMakeVisible (outputLabel);
    outputSlider.setSliderStyle (juce::Slider::LinearHorizontal); outputSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 18); outputSlider.setRange (-24, 12, 0.1); addAndMakeVisible (outputSlider);
    outputAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "OUTPUT", outputSlider);

    mixLabel.setText ("MIX", juce::dontSendNotification); mixLabel.setFont (juce::Font (juce::FontOptions (10.0f, juce::Font::bold))); addAndMakeVisible (mixLabel);
    mixSlider.setSliderStyle (juce::Slider::LinearHorizontal); mixSlider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 60, 18); mixSlider.setRange (0, 100, 1); addAndMakeVisible (mixSlider);
    mixAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (audioProcessor.apvts, "MIX", mixSlider);
}

LaPautaEQProAudioProcessorEditor::~LaPautaEQProAudioProcessorEditor() { stopTimer(); }
void LaPautaEQProAudioProcessorEditor::timerCallback() { analyserLevel = audioProcessor.analyserValue; repaint(); }

void LaPautaEQProAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff0f1115));
    auto bounds = getLocalBounds().reduced (12);
    g.setColour (juce::Colour (0xff1a1e26)); g.fillRoundedRectangle (bounds.toFloat(), 12.0f);
    g.setColour (juce::Colour (0xff00e5a0)); g.fillRoundedRectangle (juce::Rectangle<float> ((float)bounds.getX(), (float)bounds.getY(), (float)bounds.getWidth(), 3.0f), 1.5f);
    g.setColour (juce::Colour (0xff2a3140));
    for (int i=1;i<5;++i) g.drawHorizontalLine (120 + i*20, 40.0f, 860.0f);
    juce::Path curve; bool first=true;
    for (int x=40; x<=860; x+=10)
    {
        float freq = 20.0f * std::pow (1000.0f, (x-40)/820.0f);
        float y = 180.0f;
        for (int b=0;b<8;++b){ auto id="BAND"+juce::String(b+1); float bf=*audioProcessor.apvts.getRawParameterValue(id+"_FREQ"); float bg=*audioProcessor.apvts.getRawParameterValue(id+"_GAIN"); float dist=std::abs(std::log(freq/bf)); y -= bg * 4.0f * std::exp(-dist*dist*2.0f); }
        y += analyserLevel * 30.0f * std::sin (x*0.05f);
        if (first){ curve.startNewSubPath ((float)x, y); first=false; } else curve.lineTo ((float)x, y);
    }
    g.setColour (juce::Colour (0xff00e5a0));
    g.strokePath (curve, juce::PathStrokeType (2.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xff2a3140)); g.fillRect (860, 120, 12, 100);
    g.setColour (juce::Colour (0xff00e5a0)); g.fillRect (860, 220 - (int)(analyserLevel*100), 12, (int)(analyserLevel*100));
}

void LaPautaEQProAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced (20);
    titleLabel.setBounds (area.removeFromTop (32));
    subtitleLabel.setBounds (area.removeFromTop (18));
    area.removeFromTop (8); analyserLabel.setBounds (area.removeFromTop (14)); area.removeFromTop (96);
    bypassButton.setBounds (20, getHeight()-42, 110, 28); resetButton.setBounds (140, getHeight()-42, 110, 28); soloButton.setBounds (260, getHeight()-42, 80, 28); analyserButton.setBounds (350, getHeight()-42, 120, 28);
    modeLabel.setBounds (500, getHeight()-42, 50, 28); modeBox.setBounds (550, getHeight()-42, 90, 28);
    outputLabel.setBounds (650, getHeight()-42, 55, 18); outputSlider.setBounds (705, getHeight()-42, 95, 28);
    int bw = (getWidth()-40)/8;
    for (int i=0;i<8;++i){ int x=20+i*bw; bandLabels[i]->setBounds (x, 238, bw-8, 18); bandOnButtons[i]->setBounds (x+20, 256, bw-48, 18); freqSliders[i]->setBounds (x, 278, bw-8, 62); gainSliders[i]->setBounds (x, 344, bw-8, 62); qSliders[i]->setBounds (x, 410, bw-8, 52); }
    mixLabel.setBounds (20, 500, 40, 18); mixSlider.setBounds (60, 500, 150, 18);
}
