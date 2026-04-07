#include "ProjectEC1Component.h"

ProjectEC1Component::ProjectEC1Component()
{
    clearButton.setButtonText("Clear Points");
    clearButton.onClick = [this]() { canvas.clearPoints(); };

    clearDerivsButton.setButtonText("Clear Derivatives");
    clearDerivsButton.onClick = [this]() { canvas.clearAllDerivatives(); };

    addAndMakeVisible(clearButton);
    addAndMakeVisible(clearDerivsButton);
    addAndMakeVisible(canvas);

    setSize(1000, 700);
}

void ProjectEC1Component::resized()
{
    constexpr int pad      = 10;
    constexpr int toolbarH = 42;
    constexpr int gapY     = 8;

    auto area = getLocalBounds().reduced(pad);
    auto top  = area.removeFromTop(toolbarH);

    clearButton.setBounds(top.removeFromLeft(130));
    top.removeFromLeft(10);
    clearDerivsButton.setBounds(top.removeFromLeft(150));

    area.removeFromTop(gapY);
    canvas.setBounds(area);
}

void ProjectEC1Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}
