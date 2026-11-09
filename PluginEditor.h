#pragma once
#include "PluginProcessor.h"

class SpatialPannerEditor : public juce::AudioProcessorEditor,
                            private juce::Timer
{
public:
    explicit SpatialPannerEditor (SpatialPannerProcessor&);
    ~SpatialPannerEditor() override = default;

    void paint (juce::Graphics&) override;
    void resized() override {}

    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseUp   (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;

private:
    void timerCallback() override;
    void setFromMouse (juce::Point<float> p);
    void setPosition (float x, float y);
    juce::Point<float> getDotPosition() const;

    juce::Point<float> centre() const;
    float padRadius() const;

    SpatialPannerProcessor& proc;
    float curX = 0.0f, curY = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SpatialPannerEditor)
};
