#include "Project2Component.h"

Project2Component::Project2Component()
{
	// Degree
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

	// Method
	methodLabel.setText("Method", juce::dontSendNotification);
	methodLabel.setJustificationType(juce::Justification::centredLeft);

	methodBox.addItem("NLI (De Casteljau)", 1);
	methodBox.addItem("BB-form (Bernstein Sum)", 2);
	methodBox.addItem("Midpoint Subdivision", 3);
	methodBox.setSelectedId(1);

	methodBox.onChange = [this]()
		{
			const int id = methodBox.getSelectedId();
			if (id == 1) canvas.setMethod(BezierCanvas::Method::NLI);
			else if (id == 2) canvas.setMethod(BezierCanvas::Method::BB);
			else canvas.setMethod(BezierCanvas::Method::Midpoint);
		};

	// t slider 0..1 + midpoint marker
	tLabel.setText("t", juce::dontSendNotification);
	tLabel.setJustificationType(juce::Justification::centredLeft);

	tSlider.setRange(0.0, 1.0, 0.0001);
	tSlider.setValue(0.5, juce::dontSendNotification);
	tSlider.setSliderStyle(juce::Slider::LinearHorizontal);
	tSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
	tSlider.onValueChange = [this]()
		{
			canvas.setT(tSlider.getValue());
		};
	tMidpointMarker.setInterceptsMouseClicks(false, false);

	clearButton.setButtonText("Clear Points");
	clearButton.onClick = [this]()
		{
			canvas.clearPoints();
		};

	addAndMakeVisible(degreeLabel);
	addAndMakeVisible(degreeSlider);
	addAndMakeVisible(methodLabel);
	addAndMakeVisible(methodBox);
	addAndMakeVisible(tLabel);
	addAndMakeVisible(tSlider);
	addAndMakeVisible(tMidpointMarker);
	addAndMakeVisible(clearButton);
	addAndMakeVisible(canvas);

	// init defaults
	canvas.setDegree(3);
	canvas.setMethod(BezierCanvas::Method::NLI);
	canvas.setT(0.5);

	setSize(1000, 700);
}

void Project2Component::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkslategrey);

	const auto b = tSlider.getBounds();

	// getPositionOfValue returns position within the slider component
	const float x = static_cast<float>(tSlider.getX()) + tSlider.getPositionOfValue(0.5);

	g.setColour(juce::Colours::white.withAlpha(0.6f));
	g.drawLine(x, static_cast<float>(b.getY()) + 2.0f, x, static_cast<float>(b.getBottom()) - 2.0f, 1.5f);
}

void Project2Component::resized()
{
	constexpr int pad = 10;
	constexpr int toolbarH = 42;
	constexpr int gapY = 8;

	auto area = getLocalBounds().reduced(pad);
	auto top = area.removeFromTop(toolbarH);

	// To place a marker at t = 0.5
	const int x = tSlider.getX() + static_cast<int>(std::round(tSlider.getPositionOfValue(0.5)));
	const int y = tSlider.getY() + 2;
	const int h = tSlider.getHeight() - 4;

	degreeLabel.setBounds(top.removeFromLeft(60));
	degreeSlider.setBounds(top.removeFromLeft(260));

	methodLabel.setBounds(top.removeFromLeft(60));
	methodBox.setBounds(top.removeFromLeft(220));

	tLabel.setBounds(top.removeFromLeft(20));
	tSlider.setBounds(top.removeFromLeft(280));
	tMidpointMarker.setBounds(x - 1, y, 2, h);

	top.removeFromLeft(10);
	clearButton.setBounds(top.removeFromLeft(130));

	area.removeFromTop(gapY);
	canvas.setBounds(area);
}