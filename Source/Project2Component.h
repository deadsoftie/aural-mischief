#pragma once
#include <JuceHeader.h>

#include "BezierCanvas.h"

class Project2Component :public juce::Component
{
public:
	Project2Component();
	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	// UI
	juce::Label degreeLabel;
	juce::Slider degreeSlider;

	juce::Label methodLabel;
	juce::ComboBox methodBox;

	juce::Label tLabel;
	juce::Slider tSlider;
	juce::Component tMidpointMarker;

	juce::TextButton clearButton;

	// Canvas
	BezierCanvas canvas;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project2Component)
};
