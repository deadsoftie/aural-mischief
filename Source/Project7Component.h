#pragma once
#include <JuceHeader.h>
#include "BSplineCanvas.h"

class Project7Component : public juce::Component
{
public:
    Project7Component();
    void paint  (juce::Graphics& g) override;
    void resized()                   override;

private:
    juce::Label      degreeLabel;
    juce::Slider     degreeSlider;

    juce::Label      tLabel;
    juce::Slider     tSlider;

    juce::Label      knotLabel;
    juce::TextEditor knotEditor;

    juce::TextButton clearButton;

    BSplineCanvas canvas;

    void refreshSliderRange();
    void refreshKnotDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project7Component)
};
