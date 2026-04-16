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
    // ----- Toolbar row 1 -----
    juce::Label      degreeLabel;
    juce::Slider     degreeSlider;

    juce::Label      methodLabel;
    juce::ComboBox   methodBox;

    juce::Label      tLabel;
    juce::Slider     tSlider;

    juce::TextButton resetCameraButton;
    juce::TextButton clearButton;

    // ----- Coord panel (toolbar row 2) -----
    juce::Label      coordPtLabel;   // shows "P0", "P1", … or "—"
    juce::Label      xLabel, yLabel, zLabel;
    juce::TextEditor xField, yField, zField;

    int selectedPointIndex = -1;

    // Update field text from a Point3D (skips fields with keyboard focus)
    void updateCoordFields(Point3D p);

    // Read field values and push to canvas
    void commitAxis(int axis);   // axis: 0=X, 1=Y, 2=Z

    // ----- Canvas -----
    BezierCanvas3D canvas;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project8Component)
};
