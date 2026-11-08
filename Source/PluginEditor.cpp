#include "PluginEditor.h"
#include <cmath>

//==============================================================================
// La Pauta EQ Pro - Helpers
//==============================================================================

namespace
{
    constexpr int bandCount = 6;

    const juce::Colour background (0xff0c1014);
    const juce::Colour panel      (0xff151b21);
    const juce::Colour grid       (0xff29333b);
    const juce::Colour turquoise  (0xff24d6c2);
    const juce::Colour yellow     (0xffffcf55);
    const juce::Colour white      (0xffedf3f4);
    const juce::Colour muted      (0xff89969e);

    juce::String bandID(const juce::String& prefix, int band)
    {
        return prefix + juce::String(band + 1);
    }

    float parameterValue(
        LaPautaEQProAudioProcessor& processor,
        const juce::String& id,
        float fallback = 0.0f)
    {
        auto* value = processor.parameters.getRawParameterValue(id);
        return value != nullptr ? value->load() : fallback;
    }

    void changeParameter(
        LaPautaEQProAudioProcessor& processor,
        const juce::String& id,
        float value)
    {
        if (auto* parameter = processor.parameters.getParameter(id))
        {
            parameter->beginChangeGesture();
            parameter->setValueNotifyingHost(
                parameter->convertTo0to1(value));
            parameter->endChangeGesture();
        }
    }

    using EQCoefficients = juce::dsp::IIR::Coefficients<float>;

    EQCoefficients::Ptr createNotch(
        double sampleRate,
        float frequency,
        float q)
    {
        const float omega =
            juce::MathConstants<float>::twoPi
            * frequency / static_cast<float>(sampleRate);

        const float alpha = std::sin(omega) / (2.0f * q);
        const float cosine = std::cos(omega);

        return new EQCoefficients(
            1.0f,
            -2.0f * cosine,
            1.0f,
            1.0f + alpha,
            -2.0f * cosine,
            1.0f - alpha);
    }

    EQCoefficients::Ptr createCoefficients(
        int type,
        double sampleRate,
        float frequency,
        float gain,
        float q)
    {
        if (sampleRate <= 0.0 || type <= 0 || type > 7)
            return {};

        frequency = juce::jlimit(
            20.0f,
            static_cast<float>(sampleRate * 0.45),
            frequency);

        q = juce::jlimit(0.1f, 10.0f, q);

        const float gainFactor =
            juce::Decibels::decibelsToGain(gain);

        switch (type)
        {
            case 1:
                return EQCoefficients::makeLowPass(
                    sampleRate, frequency, q);

            case 2:
                return EQCoefficients::makeBandPass(
                    sampleRate, frequency, q);

            case 3:
                return EQCoefficients::makeHighPass(
                    sampleRate, frequency, q);

            case 4:
                return createNotch(sampleRate, frequency, q);

            case 5:
                return EQCoefficients::makeLowShelf(
                    sampleRate, frequency, q, gainFactor);

            case 6:
                return EQCoefficients::makePeakFilter(
                    sampleRate, frequency, q, gainFactor);

            case 7:
                return EQCoefficients::makeHighShelf(
                    sampleRate, frequency, q, gainFactor);

            default:
                return {};
        }
    }
}

//==============================================================================
// EQGraph
//==============================================================================

LaPautaEQProAudioProcessorEditor::EQGraph::EQGraph(
    LaPautaEQProAudioProcessor& p,
    LaPautaEQProAudioProcessorEditor& e)
    : processor(p), editor(e)
{
    setInterceptsMouseClicks(true, true);
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::frequencyToX(
    float frequency) const
{
    const float width = static_cast<float>(getWidth());

    const float f = juce::jlimit(20.0f, 20000.0f, frequency);

    const float normalized =
        std::log(f / 20.0f) / std::log(1000.0f);

    return normalized * width;
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::gainToY(
    float gain) const
{
    const float height = static_cast<float>(getHeight());

    return juce::jmap(
        juce::jlimit(-18.0f, 18.0f, gain),
        -18.0f, 18.0f,
        height - 12.0f, 12.0f);
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::xToFrequency(
    float x) const
{
    const float width =
        static_cast<float>(juce::jmax(1, getWidth()));

    const float normalized =
        juce::jlimit(0.0f, 1.0f, x / width);

    return 20.0f * std::pow(1000.0f, normalized);
}

//==============================================================================

float LaPautaEQProAudioProcessorEditor::EQGraph::yToGain(
    float y) const
{
    const float height =
        static_cast<float>(juce::jmax(1, getHeight()));

    const float normalized =
        1.0f - juce::jlimit(0.0f, 1.0f, y / height);

    return juce::jlimit(-18.0f, 18.0f,
        -18.0f + normalized * 36.0f);
}

//==============================================================================

int LaPautaEQProAudioProcessorEditor::EQGraph::findClosestBand(
    juce::Point<float> position) const
{
    int closest = -1;
    float minimumDistance = 28.0f;

    for (int band = 0; band < bandCount; ++band)
    {
        const float frequency = parameterValue(
            processor, bandID("FREQ", band), 1000.0f);

        const float gain = parameterValue(
            processor, bandID("GAIN", band), 0.0f);

        const float x = frequencyToX(frequency);
        const float y = gainToY(gain);

        const float distance =
            position.getDistanceFrom({ x, y });

        if (distance < minimumDistance)
        {
            minimumDistance = distance;
            closest = band;
        }
    }

    return closest;
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::updateBandFromMouse(
    int band,
    juce::Point<float> position)
{
    if (band < 0 || band >= bandCount)
        return;

    changeParameter(
        processor,
        bandID("FREQ", band),
        xToFrequency(position.x));

    changeParameter(
        processor,
        bandID("GAIN", band),
        yToGain(position.y));

    editor.updateSelectedBand();
    repaint();
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::paint(
    juce::Graphics& g)
{
    const float width = static_cast<float>(getWidth());
    const float height = static_cast<float>(getHeight());

    g.fillAll(panel);

    // Grid vertical.

    const float frequencies[] =
    {
        20.0f, 50.0f, 100.0f, 200.0f, 500.0f,
        1000.0f, 2000.0f, 5000.0f, 10000.0f, 20000.0f
    };

    g.setFont(10.0f);

    for (float frequency : frequencies)
    {
        const float x = frequencyToX(frequency);

        g.setColour(grid);
        g.drawVerticalLine(
            static_cast<int>(x), 0.0f, height);

        juce::String label;

        if (frequency >= 1000.0f)
            label = juce::String(frequency / 1000.0f, 1) + "k";
        else
            label = juce::String(static_cast<int>(frequency));

        g.setColour(muted);
        g.drawText(
            label,
            static_cast<int>(x - 20.0f),
            static_cast<int>(height - 17.0f),
            40, 14,
            juce::Justification::centred);
    }

    // Grid horizontal.

    for (int db = -18; db <= 18; db += 6)
    {
        const float y = gainToY(static_cast<float>(db));

        g.setColour(db == 0
            ? juce::Colour(0xff52636b)
            : grid);

        g.drawHorizontalLine(
            static_cast<int>(y), 0.0f, width);

        g.setColour(muted);
        g.drawText(
            juce::String(db),
            3,
            static_cast<int>(y - 8.0f),
            25, 16,
            juce::Justification::centredLeft);
    }

    // FFT spectrum.

    juce::Path spectrumPath;
    bool spectrumStarted = false;

    const double sampleRate = processor.getSampleRate();

    if (sampleRate > 0.0)
    {
        for (int x = 0; x < getWidth(); x += 2)
        {
            const float frequency =
                xToFrequency(static_cast<float>(x));

            const int fftIndex = juce::jlimit(
                0, 1023,
                juce::roundToInt(
                    frequency * 2048.0 / sampleRate));

            const float magnitude =
                processor.getSpectrumValue(fftIndex);

            const float db = juce::jlimit(
                -90.0f, 0.0f,
                juce::Decibels::gainToDecibels(
                    juce::jmax(magnitude / 2048.0f, 0.000001f)));

            const float y = juce::jmap(
                db, -90.0f, 0.0f,
                height - 8.0f, 8.0f);

            if (!spectrumStarted)
            {
                spectrumPath.startNewSubPath(
                    static_cast<float>(x), y);
                spectrumStarted = true;
            }
            else
            {
                spectrumPath.lineTo(
                    static_cast<float>(x), y);
            }
        }
    }

    if (spectrumStarted)
    {
        juce::Path fill = spectrumPath;
        fill.lineTo(width, height);
        fill.lineTo(0.0f, height);
        fill.closeSubPath();

        g.setColour(juce::Colour(0x22278dff));
        g.fillPath(fill);

        g.setColour(juce::Colour(0xff278dff));
        g.strokePath(
            spectrumPath,
            juce::PathStrokeType(1.2f));
    }

    // Combined EQ curve.

    juce::Path response;
    bool responseStarted = false;

    if (sampleRate > 0.0)
    {
        for (int x = 0; x < getWidth(); x += 2)
        {
            const float frequency =
                xToFrequency(static_cast<float>(x));

            float totalDb = 0.0f;

            for (int band = 0; band < bandCount; ++band)
            {
                const bool enabled = parameterValue(
                    processor, bandID("BANDON", band), 1.0f) > 0.5f;

                const int type = juce::roundToInt(
                    parameterValue(
                        processor, bandID("TYPE", band), 6.0f));

                if (!enabled || type == 0)
                    continue;

                auto coefficients = createCoefficients(
                    type,
                    sampleRate,
                    parameterValue(
                        processor, bandID("FREQ", band), 1000.0f),
                    parameterValue(
                        processor, bandID("GAIN", band), 0.0f),
                    parameterValue(
                        processor, bandID("Q", band), 1.0f));

                if (coefficients != nullptr)
                {
                    const float magnitude =
                        coefficients->getMagnitudeForFrequency(
                            frequency, sampleRate);

                    totalDb += juce::Decibels::gainToDecibels(
                        juce::jmax(magnitude, 0.000001f));
                }
            }

            const float y = gainToY(
                juce::jlimit(-18.0f, 18.0f, totalDb));

            if (!responseStarted)
            {
                response.startNewSubPath(
                    static_cast<float>(x), y);
                responseStarted = true;
            }
            else
            {
                response.lineTo(
                    static_cast<float>(x), y);
            }
        }
    }

    g.setColour(turquoise);
    g.strokePath(
        response,
        juce::PathStrokeType(
            2.3f,
            juce::PathStrokeType::curved,
            juce::PathStrokeType::rounded));

    // Band control points.

    for (int band = 0; band < bandCount; ++band)
    {
        const float frequency = parameterValue(
            processor, bandID("FREQ", band), 1000.0f);

        const float gain = parameterValue(
            processor, bandID("GAIN", band), 0.0f);

        const bool enabled = parameterValue(
            processor, bandID("BANDON", band), 1.0f) > 0.5f;

        const float x = frequencyToX(frequency);
        const float y = gainToY(gain);

        const bool selected = editor.selectedBand == band;
        const float radius = selected ? 8.0f : 6.0f;

        g.setColour(!enabled
            ? muted
            : (selected ? yellow : turquoise));

        g.fillEllipse(
            x - radius, y - radius,
            radius * 2.0f, radius * 2.0f);

        g.setColour(background);
        g.fillEllipse(x - 2.5f, y - 2.5f, 5.0f, 5.0f);

        if (selected)
        {
            g.setColour(yellow.withAlpha(0.35f));
            g.drawEllipse(
                x - 12.0f, y - 12.0f,
                24.0f, 24.0f, 1.0f);
        }
    }
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDown(
    const juce::MouseEvent& event)
{
    mouseDownPosition = event.position;
    draggingBand = findClosestBand(event.position);
    dragStarted = false;

    if (draggingBand >= 0)
    {
        editor.selectBand(draggingBand);
        repaint();
    }
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseDrag(
    const juce::MouseEvent& event)
{
    if (draggingBand < 0)
        return;

    if (event.position.getDistanceFrom(mouseDownPosition) > 2.0f)
        dragStarted = true;

    if (dragStarted)
        updateBandFromMouse(draggingBand, event.position);
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseUp(
    const juce::MouseEvent&)
{
    if (draggingBand >= 0 && !dragStarted)
    {
        const auto id = bandID("BANDON", draggingBand);

        const bool enabled =
            parameterValue(processor, id, 1.0f) > 0.5f;

        changeParameter(
            processor, id, enabled ? 0.0f : 1.0f);
    }

    draggingBand = -1;
    dragStarted = false;

    repaint();
}

//==============================================================================

void LaPautaEQProAudioProcessorEditor::EQGraph::mouseWheelMove(
    const juce::MouseEvent& event,
    const juce::MouseWheelDetails& wheel)
{
    int band = findClosestBand(event.position);

    if (band < 0)
        band = editor.selectedBand;

    const auto id = bandID("GAIN", band);

    const float current =
        parameterValue(processor, id, 0.0f);

    changeParameter(
        processor,
        id,
        current + wheel.deltaY * 2.0f);

    repaint();
}
