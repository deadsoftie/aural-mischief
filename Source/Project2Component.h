#pragma once
#include <JuceHeader.h>

class Project2Component :public juce::Component
{
public:
	void paint(juce::Graphics& g) override
	{
		g.fillAll(juce::Colours::black);
		g.setColour(juce::Colours::white);
		g.setFont(22.0f);
		g.drawText("Project 2 (Coming Soon)", getLocalBounds(), juce::Justification::centred);
	}
};