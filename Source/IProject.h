#pragma once
#include <JuceHeader.h>

class IProject
{
public:
	virtual ~IProject() = default;

	virtual juce::String getName() const = 0;
	virtual std::unique_ptr<juce::Component> createView() = 0;
};
