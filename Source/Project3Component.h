#pragma once
#include <JuceHeader.h>

#include "NewtonInterpCanvas.h"

class Project3Component :public juce::Component
{
public:
	Project3Component();
	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	juce::Label degreeLabel;
	juce::Slider degreeSlider;
	juce::TextButton clearButton;

	NewtonInterpCanvas canvas;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project3Component)
};
