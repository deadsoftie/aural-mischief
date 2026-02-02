#pragma once

#include <JuceHeader.h>
#include "GraphComponent.h"

class Project1Component : public juce::Component
{
public:
	Project1Component();
	~Project1Component() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	//UI
	juce::Slider degreeSlider;
	juce::Label  degreeLabel;
	juce::ComboBox methodBox;
	juce::Label  methodLabel;
	juce::TextButton resetButton;

	// Graph
	GraphComponent graph;

	int degree = 1;
	std::vector<double> a; // a0..ad, clamped to [-3,3]
	GraphComponent::EvalMethod method = GraphComponent::EvalMethod::NLI;

	std::vector<std::vector<double>> choose; // binomial coefficients

	void buildChooseTable(int maxDegree);
	void setDegree(int d);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project1Component)
};