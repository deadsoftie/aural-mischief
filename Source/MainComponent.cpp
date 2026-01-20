#include "MainComponent.h"

MainComponent::MainComponent()
{
	setSize(400, 300);
}

void MainComponent::paint(juce::Graphics& g)
{
	g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
	g.setFont(juce::Font("Times New Roman", 20.0f, juce::Font::italic));
	g.setColour(juce::Colours::white);
	g.drawText(currentSizeAsString, getLocalBounds(),juce::Justification::centred, true);

	g.drawEllipse(10, 10, 60, 30,3);
}

void MainComponent::resized()
{
	// This is called when the MainComponent is resized.
	// If you add any child components, this is where you should
	// update their positions.
	currentSizeAsString = juce::String(getWidth()) + "x" + juce::String(getHeight());
}
