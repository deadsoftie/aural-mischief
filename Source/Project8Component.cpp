#include "Project8Component.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
static void styleTextField(juce::TextEditor& ed)
{
    ed.setInputRestrictions(12, "-0123456789.");
    ed.setJustification(juce::Justification::centredLeft);
    ed.setColour(juce::TextEditor::backgroundColourId,
                 juce::Colours::black.withAlpha(0.35f));
    ed.setColour(juce::TextEditor::textColourId,     juce::Colours::white);
    ed.setColour(juce::TextEditor::outlineColourId,  juce::Colours::grey.withAlpha(0.5f));
    ed.setColour(juce::TextEditor::focusedOutlineColourId, juce::Colours::cyan);
}

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
Project8Component::Project8Component()
{
    // --- Degree ---
    degreeLabel.setText("Degree", juce::dontSendNotification);
    degreeLabel.setJustificationType(juce::Justification::centredLeft);

    degreeSlider.setRange(1, 20, 1);
    degreeSlider.setValue(3, juce::dontSendNotification);
    degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    degreeSlider.onValueChange = [this]()
        { canvas.setDegree(static_cast<int>(degreeSlider.getValue())); };

    // --- Method ---
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

    // --- t slider ---
    tLabel.setText("t", juce::dontSendNotification);
    tLabel.setJustificationType(juce::Justification::centredLeft);

    tSlider.setRange(0.0, 1.0, 0.0001);
    tSlider.setValue(0.5, juce::dontSendNotification);
    tSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    tSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 60, 20);
    tSlider.onValueChange = [this]()
        { canvas.setT(tSlider.getValue()); };

    // --- Buttons ---
    resetCameraButton.setButtonText("Reset Camera");
    resetCameraButton.onClick = [this]() { canvas.resetCamera(); };

    clearButton.setButtonText("Clear Points");
    clearButton.onClick = [this]() { canvas.clearPoints(); };

    // --- Coord panel labels ---
    coordPtLabel.setText(juce::CharPointer_UTF8("\xe2\x80\x94"), juce::dontSendNotification);
    coordPtLabel.setJustificationType(juce::Justification::centredLeft);
    coordPtLabel.setColour(juce::Label::textColourId, juce::Colours::white.withAlpha(0.6f));

    for (auto* lbl : { &xLabel, &yLabel, &zLabel })
    {
        lbl->setJustificationType(juce::Justification::centredRight);
        lbl->setColour(juce::Label::textColourId, juce::Colours::lightgrey);
    }
    xLabel.setText("X", juce::dontSendNotification);
    yLabel.setText("Y", juce::dontSendNotification);
    zLabel.setText("Z", juce::dontSendNotification);

    // --- Coord text fields ---
    styleTextField(xField);
    styleTextField(yField);
    styleTextField(zField);

    xField.setTextToShowWhenEmpty("X", juce::Colours::grey);
    yField.setTextToShowWhenEmpty("Y", juce::Colours::grey);
    zField.setTextToShowWhenEmpty("Z", juce::Colours::grey);

    xField.setEnabled(false);
    yField.setEnabled(false);
    zField.setEnabled(false);

    xField.onReturnKey  = [this]() { commitAxis(0); };
    xField.onFocusLost  = [this]() { commitAxis(0); };
    yField.onReturnKey  = [this]() { commitAxis(1); };
    yField.onFocusLost  = [this]() { commitAxis(1); };
    zField.onReturnKey  = [this]() { commitAxis(2); };
    zField.onFocusLost  = [this]() { commitAxis(2); };

    // --- Canvas callbacks ---
    canvas.onSelectionChanged = [this](int idx)
        {
            selectedPointIndex = idx;

            if (idx < 0)
            {
                coordPtLabel.setText(juce::CharPointer_UTF8("\xe2\x80\x94"), juce::dontSendNotification);
                xField.setText("", juce::dontSendNotification);
                yField.setText("", juce::dontSendNotification);
                zField.setText("", juce::dontSendNotification);
                xField.setEnabled(false);
                yField.setEnabled(false);
                zField.setEnabled(false);
            }
            else
            {
                coordPtLabel.setText("P" + juce::String(idx), juce::dontSendNotification);
                xField.setEnabled(true);
                yField.setEnabled(true);
                zField.setEnabled(true);
                updateCoordFields(canvas.getPoint(idx));
            }
        };

    canvas.onPointChanged = [this](int /*idx*/, Point3D pos)
        {
            updateCoordFields(pos);
        };

    // --- Add children ---
    addAndMakeVisible(degreeLabel);
    addAndMakeVisible(degreeSlider);
    addAndMakeVisible(methodLabel);
    addAndMakeVisible(methodBox);
    addAndMakeVisible(tLabel);
    addAndMakeVisible(tSlider);
    addAndMakeVisible(resetCameraButton);
    addAndMakeVisible(clearButton);

    addAndMakeVisible(coordPtLabel);
    addAndMakeVisible(xField);
    addAndMakeVisible(yField);
    addAndMakeVisible(zField);

    addAndMakeVisible(canvas);

    // --- Init canvas ---
    canvas.setDegree(3);
    canvas.setMethod(BezierCanvas3D::Method::NLI);
    canvas.setT(0.5);
    canvas.resetToDefaultIfEmpty();

    setSize(1000, 700);
}

// ---------------------------------------------------------------------------
// Coord helpers
// ---------------------------------------------------------------------------
void Project8Component::updateCoordFields(Point3D p)
{
    if (!xField.hasKeyboardFocus(true))
        xField.setText(juce::String(p.x, 3), juce::dontSendNotification);
    if (!yField.hasKeyboardFocus(true))
        yField.setText(juce::String(p.y, 3), juce::dontSendNotification);
    if (!zField.hasKeyboardFocus(true))
        zField.setText(juce::String(p.z, 3), juce::dontSendNotification);
}

void Project8Component::commitAxis(int axis)
{
    if (selectedPointIndex < 0 || selectedPointIndex >= canvas.getPointCount())
        return;

    Point3D p = canvas.getPoint(selectedPointIndex);

    switch (axis)
    {
    case 0: p.x = xField.getText().getFloatValue(); break;
    case 1: p.y = yField.getText().getFloatValue(); break;
    case 2: p.z = zField.getText().getFloatValue(); break;
    default: break;
    }

    canvas.setPoint(selectedPointIndex, p);

    // Refresh the field so it shows the normalised value (e.g. "1.000" not "1.")
    updateCoordFields(p);
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void Project8Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);

    // t = 0.5 tick mark
    const float x = static_cast<float>(tSlider.getX()) + tSlider.getPositionOfValue(0.5);
    const auto  b = tSlider.getBounds();
    g.setColour(juce::Colours::white.withAlpha(0.6f));
    g.drawLine(x, static_cast<float>(b.getY()) + 2.0f,
               x, static_cast<float>(b.getBottom()) - 2.0f, 1.5f);

    // Divider between toolbar rows
    const int divY = coordPtLabel.getY() - 4;
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawHorizontalLine(divY, static_cast<float>(coordPtLabel.getX()),
                         static_cast<float>(getWidth() - 10));
}

// ---------------------------------------------------------------------------
// Layout
// ---------------------------------------------------------------------------
void Project8Component::resized()
{
    constexpr int pad      = 10;
    constexpr int row1H    = 42;
    constexpr int row2H    = 36;
    constexpr int gapRows  = 4;
    constexpr int gapCanvas = 8;

    auto area = getLocalBounds().reduced(pad);

    // --- Row 1: main controls ---
    auto row1 = area.removeFromTop(row1H);

    degreeLabel .setBounds(row1.removeFromLeft(60));
    degreeSlider.setBounds(row1.removeFromLeft(250));
    methodLabel .setBounds(row1.removeFromLeft(60));
    methodBox   .setBounds(row1.removeFromLeft(220));
    tLabel      .setBounds(row1.removeFromLeft(20));
    tSlider     .setBounds(row1.removeFromLeft(240));
    row1.removeFromLeft(10);
    resetCameraButton.setBounds(row1.removeFromLeft(120));
    row1.removeFromLeft(6);
    clearButton      .setBounds(row1.removeFromLeft(110));

    area.removeFromTop(gapRows);

    // --- Row 2: coord panel ---
    auto row2 = area.removeFromTop(row2H);
    row2.removeFromTop(4);   // vertical padding

    coordPtLabel.setBounds(row2.removeFromLeft(36));
    row2.removeFromLeft(10);

    xField.setBounds(row2.removeFromLeft(72));
    row2.removeFromLeft(10);
    yField.setBounds(row2.removeFromLeft(72));
    row2.removeFromLeft(10);
    zField.setBounds(row2.removeFromLeft(72));

    // --- Canvas ---
    area.removeFromTop(gapCanvas);
    canvas.setBounds(area);
}
