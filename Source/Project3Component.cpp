#include "Project3Component.h"

Project3Component::Project3Component()
{
	degreeLabel.setText("Degree", juce::dontSendNotification);
	degreeLabel.setJustificationType(juce::Justification::centredLeft);

	degreeSlider.setRange(1, 20, 1);
	degreeSlider.setValue(3, juce::dontSendNotification);
	degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
	degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
	degreeSlider.onValueChange = [this]()
		{
			canvas.setDegree(static_cast<int>(degreeSlider.getValue()));
		};

	clearButton.setButtonText("Clear Points");
	clearButton.onClick = [this]()
		{
			canvas.clearPoints();
		};

	addAndMakeVisible(degreeLabel);
	addAndMakeVisible(degreeSlider);
	addAndMakeVisible(clearButton);
	addAndMakeVisible(canvas);

	canvas.setDegree(3);
	setSize(1000, 700);
}

void Project3Component::resized()
{
	constexpr int pad = 10;
	constexpr int toolbarH = 42;
	constexpr int gapY = 8;

	auto area = getLocalBounds().reduced(pad);
	auto top = area.removeFromTop(toolbarH);

	degreeLabel.setBounds(top.removeFromLeft(60));
	degreeSlider.setBounds(top.removeFromLeft(260));

	top.removeFromLeft(10);
	clearButton.setBounds(top.removeFromLeft(130));

	area.removeFromTop(gapY);
	canvas.setBounds(area);
}

void Project3Component::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkslategrey);
}
