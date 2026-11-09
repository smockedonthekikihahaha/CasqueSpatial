#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

// Petit filtre biquad (formules RBJ), sans allocation
struct Biquad
{
    float b0 = 1, b1 = 0, b2 = 0, a1 = 0, a2 = 0;
    float z1 = 0, z2 = 0;

    void setHighShelf (double fs, double f0, double gainDb, double S = 0.7);
    void setPeak (double fs, double f0, double gainDb, double Q);
    void copyCoeffs (const Biquad& o) { b0 = o.b0; b1 = o.b1; b2 = o.b2; a1 = o.a1; a2 = o.a2; }
    void reset() { z1 = z2 = 0; }

    inline float process (float x)
    {
        const float y = b0 * x + z1;
        z1 = b1 * x - a1 * y + z2;
        z2 = b2 * x - a2 * y;
        return y;
    }
};

// Un "cote" du casque (oreille gauche ou droite)
struct Ear
{
    static constexpr int kSize = 1024;
    std::array<float, kSize> line {};
    int w = 0;

    float lpState = 0.0f;
    float lpCoef = 0.0f;        // 0 = pas de filtre
    Biquad shelf, peak;
    float gain = 1.0f;
    float delay = 0.0f;         // en echantillons (valeur courante)

    void reset();
};

class SpatialPannerProcessor : public juce::AudioProcessor
{
public:
    SpatialPannerProcessor();
    ~SpatialPannerProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Casque Spatial 3D"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState apvts;

private:
    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    void updateTargets (float x, float y, float& targetDelayL, float& targetDelayR);

    double sr = 44100.0;
    Ear earL, earR;
    juce::SmoothedValue<float> xSm, ySm;
    std::atomic<float>* xParam = nullptr;
    std::atomic<float>* yParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatialPannerProcessor)
};
