#include "Project7Component.h"

Project7Component::Project7Component()
{
    degreeLabel.setText("Degree", juce::dontSendNotification);
    degreeLabel.setJustificationType(juce::Justification::centredLeft);
    degreeSlider.setRange(1, 10, 1);
    degreeSlider.setValue(3, juce::dontSendNotification);
    degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    degreeSlider.onValueChange = [this]()
    {
        canvas.setDegree(static_cast<int>(degreeSlider.getValue()));
    };

    tLabel.setText("t", juce::dontSendNotification);
    tLabel.setJustificationType(juce::Justification::centredLeft);
    tSlider.setRange(0.0, 1.0, 0.0001);
    tSlider.setValue(0.0, juce::dontSendNotification);
    tSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    tSlider.onValueChange = [this]() { canvas.setT(tSlider.getValue()); };

    knotLabel.setText("Knots:", juce::dontSendNotification);
    knotLabel.setJustificationType(juce::Justification::centredLeft);
    knotEditor.setMultiLine(false);
    knotEditor.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    knotEditor.setColour(juce::TextEditor::backgroundColourId, juce::Colours::darkslategrey.darker(0.3f));
    knotEditor.setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);
    knotEditor.setColour(juce::TextEditor::outlineColourId, juce::Colours::grey.withAlpha(0.5f));
    knotEditor.onReturnKey = [this]()
    {
        canvas.setKnotsFromString(knotEditor.getText());
        refreshSliderRange();
        refreshKnotDisplay();
    };

    clampedButton.setButtonText("Clamped Preset");
    clampedButton.onClick = [this]()
    {
        canvas.applyClampedPreset();
        refreshSliderRange();
        refreshKnotDisplay();
    };

    clearButton.setButtonText("Clear");
    clearButton.onClick = [this]()
    {
        canvas.clearPoints();
        refreshSliderRange();
        refreshKnotDisplay();
    };

    canvas.onStateChanged = [this]() { refreshSliderRange(); refreshKnotDisplay(); };

    addAndMakeVisible(degreeLabel);
    addAndMakeVisible(degreeSlider);
    addAndMakeVisible(tLabel);
    addAndMakeVisible(tSlider);
    addAndMakeVisible(knotLabel);
    addAndMakeVisible(knotEditor);
    addAndMakeVisible(clampedButton);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(canvas);

    canvas.setDegree(3);
    setSize(1280, 720);
}

void Project7Component::refreshSliderRange()
{
    double lo = canvas.getTMin();
    double hi = canvas.getTMax();
    if (hi <= lo) { tSlider.setRange(0.0, 1.0, 0.0001); return; }
    tSlider.setRange(lo, hi, (hi - lo) / 10000.0);
    double clamped = juce::jlimit(lo, hi, tSlider.getValue());
    tSlider.setValue(clamped, juce::dontSendNotification);
    canvas.setT(clamped);
    tLabel.setText("t  [" + juce::String(lo, 2) + ", " + juce::String(hi, 2) + "]",
                   juce::dontSendNotification);
}

void Project7Component::refreshKnotDisplay()
{
    knotEditor.setText(canvas.getKnotString(), juce::dontSendNotification);
}

void Project7Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void Project7Component::resized()
{
    constexpr int pad   = 8;
    constexpr int row1H = 36;
    constexpr int row2H = 28;
    constexpr int gapY  = 4;

    auto area = getLocalBounds().reduced(pad);

    // Row 1: degree + clamped + clear
    auto row1 = area.removeFromTop(row1H);
    degreeLabel .setBounds(row1.removeFromLeft(52));
    degreeSlider.setBounds(row1.removeFromLeft(200));
    row1.removeFromLeft(8);
    clampedButton.setBounds(row1.removeFromLeft(120));
    row1.removeFromLeft(8);
    clearButton  .setBounds(row1.removeFromLeft(70));

    area.removeFromTop(gapY);

    // Row 2: t slider
    auto row2 = area.removeFromTop(row2H);
    tLabel .setBounds(row2.removeFromLeft(140));
    tSlider.setBounds(row2);

    area.removeFromTop(gapY);

    // Row 3: knot display
    auto row3 = area.removeFromTop(row2H);
    knotLabel .setBounds(row3.removeFromLeft(46));
    knotEditor.setBounds(row3);

    area.removeFromTop(gapY);

    canvas.setBounds(area);
}
