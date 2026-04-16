#include "Project8Component.h"

Project8Component::Project8Component()
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
            if      (id == 1) canvas.setMethod(BezierCanvas3D::Method::NLI);
            else if (id == 2) canvas.setMethod(BezierCanvas3D::Method::BB);
            else              canvas.setMethod(BezierCanvas3D::Method::Midpoint);
        };

    // t slider
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

    // Buttons
    resetCameraButton.setButtonText("Reset Camera");
    resetCameraButton.onClick = [this]()
        {
            canvas.resetCamera();
        };

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
    addAndMakeVisible(resetCameraButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(canvas);

    // Init canvas defaults
    canvas.setDegree(3);
    canvas.setMethod(BezierCanvas3D::Method::NLI);
    canvas.setT(0.5);
    canvas.resetToDefaultIfEmpty();

    setSize(1000, 700);
}

void Project8Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);

    // t = 0.5 tick mark on the slider
    const float x = static_cast<float>(tSlider.getX()) + tSlider.getPositionOfValue(0.5);
    const auto  b = tSlider.getBounds();
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawLine(x, static_cast<float>(b.getY()) + 2.0f,
               x, static_cast<float>(b.getBottom()) - 2.0f, 1.5f);
}

void Project8Component::resized()
{
    constexpr int pad     = 10;
    constexpr int toolbarH = 42;
    constexpr int gapY    = 8;

    auto area = getLocalBounds().reduced(pad);
    auto top  = area.removeFromTop(toolbarH);

    degreeLabel .setBounds(top.removeFromLeft(60));
    degreeSlider.setBounds(top.removeFromLeft(250));

    methodLabel.setBounds(top.removeFromLeft(60));
    methodBox  .setBounds(top.removeFromLeft(220));

    tLabel .setBounds(top.removeFromLeft(20));
    tSlider.setBounds(top.removeFromLeft(240));

    top.removeFromLeft(10);
    resetCameraButton.setBounds(top.removeFromLeft(120));
    top.removeFromLeft(6);
    clearButton      .setBounds(top.removeFromLeft(110));

    area.removeFromTop(gapY);
    canvas.setBounds(area);
}
