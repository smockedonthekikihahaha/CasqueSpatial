#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cmath>

//==============================================================================
void Biquad::setHighShelf (double fs, double f0, double gainDb, double S)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = juce::MathConstants<double>::twoPi * f0 / fs;
    const double cw = std::cos (w0), sw = std::sin (w0);
    const double alpha = sw / 2.0 * std::sqrt ((A + 1.0 / A) * (1.0 / S - 1.0) + 2.0);
    const double sqA = 2.0 * std::sqrt (A) * alpha;

    const double B0 =      A * ((A + 1) + (A - 1) * cw + sqA);
    const double B1 = -2 * A * ((A - 1) + (A + 1) * cw);
    const double B2 =      A * ((A + 1) + (A - 1) * cw - sqA);
    const double A0 =           (A + 1) - (A - 1) * cw + sqA;
    const double A1 =  2 *     ((A - 1) - (A + 1) * cw);
    const double A2 =           (A + 1) - (A - 1) * cw - sqA;

    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
    a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}

void Biquad::setPeak (double fs, double f0, double gainDb, double Q)
{
    const double A = std::pow (10.0, gainDb / 40.0);
    const double w0 = juce::MathConstants<double>::twoPi * f0 / fs;
    const double cw = std::cos (w0), sw = std::sin (w0);
    const double alpha = sw / (2.0 * Q);

    const double B0 = 1 + alpha * A, B1 = -2 * cw, B2 = 1 - alpha * A;
    const double A0 = 1 + alpha / A, A1 = -2 * cw, A2 = 1 - alpha / A;

    b0 = (float) (B0 / A0); b1 = (float) (B1 / A0); b2 = (float) (B2 / A0);
    a1 = (float) (A1 / A0); a2 = (float) (A2 / A0);
}

void Ear::reset()
{
    line.fill (0.0f);
    w = 0; lpState = 0.0f; delay = 0.0f;
    shelf.reset(); peak.reset();
}

//==============================================================================
SpatialPannerProcessor::SpatialPannerProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
    xParam = apvts.getRawParameterValue ("x");
    yParam = apvts.getRawParameterValue ("y");
}

juce::AudioProcessorValueTreeState::ParameterLayout SpatialPannerProcessor::createLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    // x : -1 = gauche, +1 = droite   |   y : -1 = derriere, +1 = devant
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "x", 1 }, "Gauche / Droite",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 0.0f));
    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "y", 1 }, "Arriere / Avant",
        juce::NormalisableRange<float> (-1.0f, 1.0f, 0.001f), 1.0f * 0.0f));
    return layout;
}

bool SpatialPannerProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    return l.getMainInputChannelSet()  == juce::AudioChannelSet::stereo()
        && l.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}

void SpatialPannerProcessor::prepareToPlay (double sampleRate, int)
{
    sr = sampleRate;
    earL.reset(); earR.reset();
    xSm.reset (sampleRate, 0.05);
    ySm.reset (sampleRate, 0.05);
    xSm.setCurrentAndTargetValue (xParam->load());
    ySm.setCurrentAndTargetValue (yParam->load());

    float dl, dr;
    updateTargets (xSm.getCurrentValue(), ySm.getCurrentValue(), dl, dr);
    earL.delay = dl;
    earR.delay = dr;
}

//==============================================================================
// Coeur du plugin : transforme la position (x,y) en reglages d'oreilles
//   - ITD  : delai entre les deux oreilles (le son arrive plus tot d'un cote)
//   - ILD  : oreille opposee plus faible + filtree (l'ombre de la tete)
//   - Avant/arriere : derriere = aigus attenues (l'ombre des pavillons), devant = leger boost presence
//   - Distance : plus le point est loin du casque, plus c'est faible et sombre
void SpatialPannerProcessor::updateTargets (float x, float y, float& targetDelayL, float& targetDelayR)
{
    const float r = std::min (1.0f, std::sqrt (x * x + y * y));
    const float az = std::atan2 (x, y);                        // 0 = devant, +90deg = droite, 180deg = derriere
    const float amount = std::min (1.0f, r / 0.3f);            // au centre de la tete : pas d'effet

    const float s = std::sin (az) * amount;                    // lateral (-1 gauche .. +1 droite)
    const float c = std::cos (az) * amount;                    // +1 devant .. -1 derriere
    const float absS = std::abs (s);
    const float rear  = std::max (0.0f, -c);
    const float front = std::max (0.0f,  c);

    // --- ITD (max ~0.66 ms) ---
    const float itd = 0.00066f * (float) sr * s;
    targetDelayL = itd > 0 ? itd : 0.0f;                       // source a droite -> oreille gauche en retard
    targetDelayR = itd < 0 ? -itd : 0.0f;

    // --- ILD + ombre de la tete (filtre passe-bas sur l'oreille opposee) ---
    const float farCutoff = std::min (20000.0f * std::pow (0.1f, absS), 0.45f * (float) sr);   // 20k -> 2k Hz
    const float farCoef = std::exp (-juce::MathConstants<float>::twoPi * farCutoff / (float) sr);
    const float farGain  = juce::Decibels::decibelsToGain (-6.0f * absS);
    const float nearGain = juce::Decibels::decibelsToGain ( 1.5f * absS);

    const bool rightIsNear = s > 0;
    const bool leftIsNear  = s < 0;
    earL.lpCoef = leftIsNear ? 0.0f : (absS > 0.01f ? farCoef : 0.0f);
    earR.lpCoef = rightIsNear ? 0.0f : (absS > 0.01f ? farCoef : 0.0f);

    const float distGain = 1.0f - 0.6f * r;
    earL.gain = distGain * (leftIsNear  ? nearGain : (rightIsNear ? farGain : 1.0f));
    earR.gain = distGain * (rightIsNear ? nearGain : (leftIsNear  ? farGain : 1.0f));

    // --- Avant / arriere + distance (memes filtres sur les deux oreilles) ---
    earL.shelf.setHighShelf (sr, 4500.0, -9.0 * rear - 4.0 * r);
    earL.peak.setPeak       (sr, 3000.0,  2.5 * front, 1.0);
    earR.shelf.copyCoeffs (earL.shelf);
    earR.peak.copyCoeffs  (earL.peak);
}

void SpatialPannerProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int n = buffer.getNumSamples();
    if (buffer.getNumChannels() < 2) return;

    float* L = buffer.getWritePointer (0);
    float* R = buffer.getWritePointer (1);

    xSm.setTargetValue (xParam->load());
    ySm.setTargetValue (yParam->load());

    constexpr int kSub = 32;
    constexpr int mask = Ear::kSize - 1;

    int pos = 0;
    while (pos < n)
    {
        const int len = std::min (kSub, n - pos);
        const float x = xSm.skip (len);
        const float y = ySm.skip (len);

        float tgtDL, tgtDR;
        updateTargets (x, y, tgtDL, tgtDR);

        const float startDL = earL.delay, startDR = earR.delay;

        for (int i = 0; i < len; ++i)
        {
            const float mono = 0.5f * (L[pos + i] + R[pos + i]);
            const float t = (float) (i + 1) / (float) len;

            auto processEar = [&] (Ear& e, float d) -> float
            {
                e.line[(size_t) e.w] = mono;

                const int   di   = (int) d;
                const float frac = d - (float) di;
                const float s0 = e.line[(size_t) ((e.w - di)     & mask)];
                const float s1 = e.line[(size_t) ((e.w - di - 1) & mask)];
                float v = s0 + (s1 - s0) * frac;
                e.w = (e.w + 1) & mask;

                // ombre de la tete (passe-bas 1 pole)
                if (e.lpCoef > 0.0f)
                {
                    e.lpState = (1.0f - e.lpCoef) * v + e.lpCoef * e.lpState;
                    v = e.lpState;
                }
                else
                    e.lpState = v;

                v = e.shelf.process (v);
                v = e.peak.process (v);
                return v * e.gain;
            };

            L[pos + i] = processEar (earL, startDL + (tgtDL - startDL) * t);
            R[pos + i] = processEar (earR, startDR + (tgtDR - startDR) * t);
        }

        earL.delay = tgtDL;
        earR.delay = tgtDR;
        pos += len;
    }
}

//==============================================================================
juce::AudioProcessorEditor* SpatialPannerProcessor::createEditor()
{
    return new SpatialPannerEditor (*this);
}

void SpatialPannerProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void SpatialPannerProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpatialPannerProcessor();
}
