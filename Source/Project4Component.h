#pragma once
#include <JuceHeader.h>

#include "CubicSplineCanvas.h"

class Project4Component : public juce::Component
{
public:
    Project4Component();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::Label      segmentsLabel;
    juce::Slider     segmentsSlider;
    juce::TextButton clearButton;
    CubicSplineCanvas canvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project4Component)
};
