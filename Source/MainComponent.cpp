#include "MainComponent.h"
#include "GraphComponent.h"

MainComponent::MainComponent()
{
	// Building the binomial coeffs up to 20
	buildChooseTable(20);

	// Degree slider
	degreeLabel.setText("Degree", juce::dontSendNotification);
	degreeLabel.setJustificationType(juce::Justification::centredLeft);

	degreeSlider.setRange(1, 20, 1);
	degreeSlider.setValue(1, juce::dontSendNotification);
	degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
	degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
	degreeSlider.setSkewFactor(1.0);

	degreeSlider.onValueChange = [this]()
		{
			const int d = static_cast<int>(degreeSlider.getValue());
			setDegree(d);
		};

	// Method dropdown
	methodLabel.setText("Method", juce::dontSendNotification);
	methodLabel.setJustificationType(juce::Justification::centredLeft);
	methodBox.addItem("NLI (De Casteljau)", 1);
	methodBox.addItem("BB-form (Bernstein Sum)", 2);
	methodBox.setSelectedId(1);

	methodBox.onChange = [this]()
		{
			method = (methodBox.getSelectedId() == 1) ? GraphComponent::EvalMethod::NLI : GraphComponent::EvalMethod::BB;
			graph.setMethod(method);
		};

	resetButton.setButtonText("Reset");
	resetButton.onClick = [this]()
		{
			std::fill(a.begin(), a.end(), 1.0);
			graph.repaint();
		};

	addAndMakeVisible(degreeLabel);
	addAndMakeVisible(degreeSlider);
	addAndMakeVisible(methodLabel);
	addAndMakeVisible(methodBox);
	addAndMakeVisible(resetButton);
	addAndMakeVisible(graph);

	// Default state: d=1, a_i=1
	setDegree(1);

	setSize(900, 600);
}

void MainComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkslategrey);
}

void MainComponent::resized()
{
	constexpr int pad = 10;
	constexpr int toolbarH = 40;
	constexpr int gapX = 20;
	constexpr int gapY = 10;

	auto area = getLocalBounds().reduced(pad);
	auto top = area.removeFromTop(toolbarH);

	degreeLabel.setBounds(top.removeFromLeft(70));
	degreeSlider.setBounds(top.removeFromLeft(300));

	methodLabel.setBounds(top.removeFromLeft(70));
	methodBox.setBounds(top.removeFromLeft(190));

	top.removeFromLeft(gapX);
	resetButton.setBounds(top.removeFromLeft(120));

	area.removeFromTop(gapY);
	graph.setBounds(area);
}

void MainComponent::buildChooseTable(int maxDegree)
{
	choose.assign(static_cast<size_t>(maxDegree) + 1, std::vector<double>(static_cast<size_t>(maxDegree) + 1, 0.0));

	for (int n = 0; n <= maxDegree; ++n)
	{
		choose[static_cast<size_t>(n)][0] = 1.0;
		choose[static_cast<size_t>(n)][static_cast<size_t>(n)] = 1.0;

		for (int k = 1; k < n; ++k)
			choose[static_cast<size_t>(n)][static_cast<size_t>(k)] =
			choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k - 1)] + choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k)];
	}
}

void MainComponent::setDegree(int d)
{
	degree = juce::jlimit(1, 20, d);

	// Reset coeffs for new degree to a_i = 1
	a.assign(static_cast<size_t>(degree) + 1, 1.0);

	graph.setModel(degree, &a, method, &choose);
}
