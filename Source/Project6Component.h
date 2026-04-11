#pragma once
#include <JuceHeader.h>
#include "DeBoorPolyCanvas.h"

class Project6Component : public juce::Component
{
public:
    Project6Component();

    void paint  (juce::Graphics& g) override;
    void resized()                   override;

private:
    // UI controls
    juce::Label    degreeLabel;
    juce::Slider   degreeSlider;

    juce::ComboBox modeBox;
    juce::TextButton clearButton;

    juce::Label      knotLabel;
    juce::TextEditor knotDisplay;

    // Canvas
    DeBoorPolyCanvas canvas;

    void updateKnotDisplay();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project6Component)
};
