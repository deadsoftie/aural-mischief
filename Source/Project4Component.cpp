#include "Project4Component.h"

Project4Component::Project4Component()
{
    segmentsLabel.setText("Segments", juce::dontSendNotification);
    segmentsLabel.setJustificationType(juce::Justification::centredLeft);

    segmentsSlider.setRange(1, 20, 1);
    segmentsSlider.setValue(3, juce::dontSendNotification);
    segmentsSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    segmentsSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    segmentsSlider.onValueChange = [this]()
        {
            canvas.setSegments(static_cast<int>(segmentsSlider.getValue()));
        };

    clearButton.setButtonText("Clear Points");
    clearButton.onClick = [this]()
        {
            canvas.clearPoints();
        };

    addAndMakeVisible(segmentsLabel);
    addAndMakeVisible(segmentsSlider);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(canvas);

    canvas.setSegments(3);
    setSize(1000, 700);
}

void Project4Component::resized()
{
    constexpr int pad     = 10;
    constexpr int toolbarH = 42;
    constexpr int gapY    = 8;

    auto area = getLocalBounds().reduced(pad);
    auto top  = area.removeFromTop(toolbarH);

    segmentsLabel.setBounds(top.removeFromLeft(70));
    segmentsSlider.setBounds(top.removeFromLeft(260));

    top.removeFromLeft(10);
    clearButton.setBounds(top.removeFromLeft(130));

    area.removeFromTop(gapY);
    canvas.setBounds(area);
}

void Project4Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}
