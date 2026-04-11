#include "Project6Component.h"

Project6Component::Project6Component()
{
    // ── Degree slider ─────────────────────────────────────────────────────────
    degreeLabel.setText("Degree", juce::dontSendNotification);
    degreeLabel.setJustificationType(juce::Justification::centredLeft);

    degreeSlider.setRange(1, 20, 1);
    degreeSlider.setValue(3, juce::dontSendNotification);
    degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 40, 20);
    degreeSlider.onValueChange = [this]()
    {
        canvas.setDegree(static_cast<int>(degreeSlider.getValue()));
        modeBox.setSelectedId(1, juce::dontSendNotification);
        updateKnotDisplay();
    };

    // ── Mode combo box ────────────────────────────────────────────────────────
    modeBox.addItem("Bezier Mode",   1);
    modeBox.addItem("De Boor Mode",  2);
    modeBox.setSelectedId(1, juce::dontSendNotification);

    modeBox.onChange = [this]()
    {
        const bool wantDeBoor = (modeBox.getSelectedId() == 2);

        if (wantDeBoor == canvas.isDeBoorMode())
            return;

        canvas.switchKnots();

        // If switchKnots() declined (not enough points), revert the selection
        if (wantDeBoor != canvas.isDeBoorMode())
            modeBox.setSelectedId(1, juce::dontSendNotification);

        updateKnotDisplay();
    };

    // ── Clear button ──────────────────────────────────────────────────────────
    clearButton.setButtonText("Clear");
    clearButton.onClick = [this]()
    {
        canvas.clearPoints();
        modeBox.setSelectedId(1, juce::dontSendNotification);
        updateKnotDisplay();
    };

    // ── Knot sequence display ─────────────────────────────────────────────────
    knotLabel.setText("Knots:", juce::dontSendNotification);
    knotLabel.setJustificationType(juce::Justification::centredLeft);

    knotDisplay.setReadOnly(true);
    knotDisplay.setMultiLine(false);
    knotDisplay.setFont(juce::Font(juce::Font::getDefaultMonospacedFontName(), 12.0f, juce::Font::plain));
    knotDisplay.setColour(juce::TextEditor::backgroundColourId, juce::Colours::darkslategrey.darker(0.3f));
    knotDisplay.setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);
    knotDisplay.setColour(juce::TextEditor::outlineColourId, juce::Colours::grey.withAlpha(0.5f));

    // Register children
    addAndMakeVisible(degreeLabel);
    addAndMakeVisible(degreeSlider);
    addAndMakeVisible(modeBox);
    addAndMakeVisible(clearButton);
    addAndMakeVisible(knotLabel);
    addAndMakeVisible(knotDisplay);
    addAndMakeVisible(canvas);

    // Initial state
    canvas.setDegree(3);
    canvas.onKnotsChanged = [this]() { updateKnotDisplay(); };
    updateKnotDisplay();

    setSize(1280, 720);
}

void Project6Component::updateKnotDisplay()
{
    knotDisplay.setText(canvas.getKnotString(), juce::dontSendNotification);
}

void Project6Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);
}

void Project6Component::resized()
{
    constexpr int pad   = 8;
    constexpr int row1H = 36;
    constexpr int row2H = 26;
    constexpr int gapY  = 4;

    auto area = getLocalBounds().reduced(pad);

    // ── Row 1: degree + mode combo + clear ───────────────────────────────────
    auto row1 = area.removeFromTop(row1H);

    degreeLabel .setBounds(row1.removeFromLeft(52));
    degreeSlider.setBounds(row1.removeFromLeft(220));
    row1.removeFromLeft(8);

    modeBox.setBounds(row1.removeFromLeft(150));
    row1.removeFromLeft(8);

    clearButton.setBounds(row1.removeFromLeft(70));

    area.removeFromTop(gapY);

    // ── Row 2: knot sequence display ──────────────────────────────────────────
    auto row2 = area.removeFromTop(row2H);
    knotLabel  .setBounds(row2.removeFromLeft(46));
    knotDisplay.setBounds(row2);

    area.removeFromTop(gapY);

    // ── Canvas fills the rest ─────────────────────────────────────────────────
    canvas.setBounds(area);
}
