#pragma once

#include <JuceHeader.h>

/*
	This component lives inside our window, and this is where you should put all
	your controls and content.
*/
class MainComponent : public juce::Component
{
public:
	MainComponent();
	~MainComponent() override = default;

	void paint(juce::Graphics&) override;
	void resized() override;

private:
	juce::String currentSizeAsString;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
