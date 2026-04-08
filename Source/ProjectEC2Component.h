#pragma once
#include <JuceHeader.h>
#include "BestFitCanvas.h"

class ProjectEC2Component : public juce::Component
{
public:
    ProjectEC2Component();
    void paint  (juce::Graphics&) override;
    void resized() override;

private:
    juce::ComboBox   modeBox;
    juce::TextButton clearButton;
    BestFitCanvas    canvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEC2Component)
};
