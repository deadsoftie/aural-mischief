#include "ProjectEC2Component.h"

ProjectEC2Component::ProjectEC2Component()
{
    modeBox.addItem("Line",     1);
    modeBox.addItem("Parabola", 2);
    modeBox.setSelectedId(1, juce::dontSendNotification);
    modeBox.onChange = [this]() {
        canvas.setMode(modeBox.getSelectedId() == 2);
    };

    clearButton.setButtonText("Clear All");
    clearButton.onClick = [this]() { canvas.clearPoints(); };

    addAndMakeVisible(modeBox);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(canvas);

    setSize(1000, 700);
}

void ProjectEC2Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void ProjectEC2Component::resized()
{
    constexpr int pad     = 10;
    constexpr int toolbarH = 42;
    constexpr int gapY    = 8;

    auto area = getLocalBounds().reduced(pad);
    auto top  = area.removeFromTop(toolbarH);

    modeBox    .setBounds(top.removeFromLeft(120));
    top.removeFromLeft(10);
    clearButton.setBounds(top.removeFromLeft(100));

    area.removeFromTop(gapY);
    canvas.setBounds(area);
}
