#include "MainComponent.h"
#include "GraphComponent.h"

MainComponent::MainComponent()
{
	// Building the binomial coeffs up to 20
	buildChooseTable(20);

	// Degree dropdown
	for (int d = 1; d <= 20; ++d)
		degreeBox.addItem(juce::String(d), d);

	degreeBox.setSelectedId(1);

	degreeBox.onChange = [this]()
		{
			setDegree(degreeBox.getSelectedId());
		};

	// Method dropdown
	methodBox.addItem("NLI (De Casteljau)", 1);
	methodBox.addItem("BB-form (Bernstein Sum)", 2);
	methodBox.setSelectedId(1);

	methodBox.onChange = [this]()
		{
			method = (methodBox.getSelectedId() == 1) ? GraphComponent::EvalMethod::NLI : GraphComponent::EvalMethod::BB;
		};

	resetButton.setButtonText("Reset");
	resetButton.onClick = [this]()
		{
			std::fill(a.begin(), a.end(), 1.0);
			graph.repaint();
		};

	addAndMakeVisible(degreeBox);
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
	auto r = getLocalBounds().reduced(10);
	auto top = r.removeFromTop(40);

	degreeBox.setBounds(top.removeFromLeft(120));
	methodBox.setBounds(top.removeFromLeft(260));
	resetButton.setBounds(top.removeFromLeft(160));
	graph.setBounds(r);
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
