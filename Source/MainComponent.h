#pragma once

#include <JuceHeader.h>

#include "GraphComponent.h"
#include "GraphComponent.h"

class MainComponent : public juce::Component
{
public:
	MainComponent();
	~MainComponent() override = default;

	void paint(juce::Graphics& g) override;
	void resized() override;

private:
	// UI
	juce::ComboBox degreeBox;
	juce::ComboBox methodBox;
	juce::TextButton resetButton;

	// Graph
	GraphComponent graph;

	int degree = 1;
	std::vector<double> a; // a0..ad, clamped to [-3,3]
	GraphComponent::EvalMethod method = GraphComponent::EvalMethod::NLI;

	std::vector<std::vector<double>> choose; // binomial co-efficients

	void buildChooseTable(int maxDegree);
	void setDegree(int d);

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent)
};
