#pragma once
#include <JuceHeader.h>
#include "CatmullRomCanvas.h"

class ProjectGP1Component : public juce::Component
{
public:
    ProjectGP1Component();
    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Toolbar controls
    juce::Label     paramLabel;
    juce::ComboBox  paramBox;

    juce::ToggleButton tangentsButton  { "Show Tangents" };
    juce::ToggleButton bezierButton    { "Bezier Equivalent" };
    juce::ToggleButton extendButton    { "Extend to Endpoints" };

    juce::TextButton clearButton       { "Clear" };

    juce::Label     segmentLabel;

    // Canvas
    CatmullRomCanvas canvas;

    void updateSegmentLabel();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ProjectGP1Component)
};
