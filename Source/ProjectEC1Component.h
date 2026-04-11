#pragma once
#include <JuceHeader.h>
#include "HermiteInterpCanvas.h"

class ProjectEC1Component : public juce::Component
{
public:
    ProjectEC1Component();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    juce::TextButton      clearButton;
    juce::TextButton      clearDerivsButton;
    HermiteInterpCanvas   canvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectEC1Component)
};
