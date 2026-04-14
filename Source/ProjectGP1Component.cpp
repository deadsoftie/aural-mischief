#include "ProjectGP1Component.h"

ProjectGP1Component::ProjectGP1Component()
{
    // Parameterization combo
    paramLabel.setText("Parameterization", juce::dontSendNotification);
    paramLabel.setJustificationType(juce::Justification::centredLeft);

    paramBox.addItem("Uniform", 1);
    paramBox.addItem("Centripetal (default)", 2);
    paramBox.addItem("Chordal", 3);
    paramBox.setSelectedId(2);
    paramBox.onChange = [this]()
    {
        const int id = paramBox.getSelectedId();
        if (id == 1) canvas.setParameterization(CatmullRomCanvas::Parameterization::Uniform);
        else if (id == 3) canvas.setParameterization(CatmullRomCanvas::Parameterization::Chordal);
        else canvas.setParameterization(CatmullRomCanvas::Parameterization::Centripetal);
        updateSegmentLabel();
    };

    // Tangents toggle
    tangentsButton.setToggleState(false, juce::dontSendNotification);
    tangentsButton.onClick = [this]()
    {
        canvas.setShowTangents(tangentsButton.getToggleState());
    };

    // Bezier equivalent toggle
    bezierButton.setToggleState(false, juce::dontSendNotification);
    bezierButton.onClick = [this]()
    {
        canvas.setShowBezierEquivalent(bezierButton.getToggleState());
    };

    // Extend to endpoints toggle
    extendButton.setToggleState(false, juce::dontSendNotification);
    extendButton.onClick = [this]()
    {
        canvas.setExtendToEndpoints(extendButton.getToggleState());
        updateSegmentLabel();
    };

    // Clear button
    clearButton.onClick = [this]()
    {
        canvas.clearPoints();
    };

    // Segment count label
    segmentLabel.setText("Segments: 0", juce::dontSendNotification);
    segmentLabel.setJustificationType(juce::Justification::centredLeft);

    canvas.onPointsChanged = [this]() { updateSegmentLabel(); };

    addAndMakeVisible(paramLabel);
    addAndMakeVisible(paramBox);
    addAndMakeVisible(tangentsButton);
    addAndMakeVisible(bezierButton);
    addAndMakeVisible(extendButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(segmentLabel);
    addAndMakeVisible(canvas);

    setSize(1280, 720);
}

void ProjectGP1Component::updateSegmentLabel()
{
    segmentLabel.setText("Segments: " + juce::String(canvas.getSegmentCount()),
        juce::dontSendNotification);
}

void ProjectGP1Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void ProjectGP1Component::resized()
{
    constexpr int pad = 8;
    constexpr int toolbarH = 42;
    constexpr int gapY = 6;

    auto area = getLocalBounds().reduced(pad);
    auto top = area.removeFromTop(toolbarH);

    paramLabel.setBounds(top.removeFromLeft(120));
    paramBox.setBounds(top.removeFromLeft(180));
    top.removeFromLeft(8);
    tangentsButton.setBounds(top.removeFromLeft(120));
    top.removeFromLeft(4);
    bezierButton.setBounds(top.removeFromLeft(140));
    top.removeFromLeft(4);
    extendButton.setBounds(top.removeFromLeft(160));
    top.removeFromLeft(8);
    clearButton.setBounds(top.removeFromLeft(60));
    top.removeFromLeft(12);
    segmentLabel.setBounds(top.removeFromLeft(120));

    area.removeFromTop(gapY);
    canvas.setBounds(area);
}
