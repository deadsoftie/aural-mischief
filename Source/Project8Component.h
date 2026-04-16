#pragma once
#include <JuceHeader.h>
#include "BezierCanvas3D.h"

class Project8Component : public juce::Component
{
public:
    Project8Component();
    void paint(juce::Graphics& g) override;
    void resized()                override;

private:
    juce::Label      degreeLabel;
    juce::Slider     degreeSlider;

    juce::Label      methodLabel;
    juce::ComboBox   methodBox;

    juce::Label      tLabel;
    juce::Slider     tSlider;

    juce::TextButton resetCameraButton;
    juce::TextButton clearButton;

    BezierCanvas3D   canvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project8Component)
};
