/*
  ==============================================================================

    Project5Component.cpp
    Created: 4 Apr 2026 8:47:19pm
    Author:  rahul

  ==============================================================================
*/

#include "Project5Component.h"

// ============================================================================
// Construction
// ============================================================================

Project5Component::Project5Component()
{
    // ── Degree slider ────────────────────────────────────────────────────────
    degreeLabel.setText("Degree:", juce::dontSendNotification);
    degreeLabel.setJustificationType(juce::Justification::centredLeft);

    degreeSlider.setRange(1, 20, 1);
    degreeSlider.setValue(d, juce::dontSendNotification);
    degreeSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    degreeSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, 20);
    degreeSlider.onValueChange = [this]()
    {
        d = static_cast<int>(degreeSlider.getValue());
        // Ensure active interval [d, N-d] is non-trivial (width >= 2)
        N = std::max(d + 10, 2 * d + 2);
        nSlider.setRange(2 * d + 2, 2 * d + 20, 1);
        nSlider.setValue(N, juce::dontSendNotification);
        const int nc = numCoeffs();
        coeffs.assign(static_cast<size_t>(std::max(nc, 0)), 1.0);
        repaint();
    };

    // ── N slider ──────────────────────────────────────────────────────────────
    nLabel.setText("N:", juce::dontSendNotification);
    nLabel.setJustificationType(juce::Justification::centredLeft);

    nSlider.setRange(2 * d + 2, 2 * d + 20, 1);
    nSlider.setValue(N, juce::dontSendNotification);
    nSlider.setSliderStyle(juce::Slider::LinearHorizontal);
    nSlider.setTextBoxStyle(juce::Slider::TextBoxLeft, false, 30, 20);
    nSlider.onValueChange = [this]()
    {
        N = static_cast<int>(nSlider.getValue());
        rebuild();   // preserves existing coeffs, adds 1.0 for new ones
        repaint();
    };

    // ── Method dropdown ───────────────────────────────────────────────────────
    methodLabel.setText("Method:", juce::dontSendNotification);
    methodLabel.setJustificationType(juce::Justification::centredLeft);

    methodBox.addItem("De Boor", 1);
    methodBox.addItem("Divided Differences", 2);
    methodBox.setSelectedId(1);
    methodBox.onChange = [this]()
    {
        method = (methodBox.getSelectedId() == 2) ? EvalMethod::DividedDiff : EvalMethod::DeBoor;
        repaint();
    };

    // ── Reset button ──────────────────────────────────────────────────────────
    resetButton.setButtonText("Reset");
    resetButton.onClick = [this]()
    {
        std::fill(coeffs.begin(), coeffs.end(), 1.0);
        repaint();
    };

    addAndMakeVisible(degreeLabel);
    addAndMakeVisible(degreeSlider);
    addAndMakeVisible(nLabel);
    addAndMakeVisible(nSlider);
    addAndMakeVisible(methodLabel);
    addAndMakeVisible(methodBox);
    addAndMakeVisible(resetButton);

    coeffs.assign(static_cast<size_t>(numCoeffs()), 1.0);
    setSize(1200, 700);
}

// ============================================================================
// State management
// ============================================================================

void Project5Component::rebuild()
{
    const int nc = numCoeffs();
    if (nc <= 0) { coeffs.clear(); return; }
    // Preserve existing coefficients; fill any new ones with 1.0
    coeffs.resize(static_cast<size_t>(nc), 1.0);
}

// ============================================================================
// Evaluation  — uniform knots t_k = k
// ============================================================================

// De Boor algorithm (Nested Linear Interpolation on knot intervals).
// For t in [d, N-d], sets J = floor(t) then runs d stages of blending.
double Project5Component::evalDeBoor(double t) const
{
    const int nc = numCoeffs();
    if (nc <= 0) return 0.0;

    // J is the knot-interval index: t in [J, J+1). Clamp right endpoint.
    int J = static_cast<int>(std::floor(t));
    J = juce::jlimit(d, N - d - 1, J);

    // Work copy — indices 0..nc-1 map directly to global coefficient indices
    std::vector<double> c(coeffs.begin(), coeffs.end());

    // d stages; right-to-left traversal keeps c[i-1] at stage (p-1) when needed
    for (int p = 1; p <= d; ++p)
    {
        const double denom = static_cast<double>(d - p + 1);
        for (int i = J; i >= J - d + p; --i)
        {
            const double alpha = (t - static_cast<double>(i)) / denom;
            const double cprev = (i > 0) ? c[static_cast<size_t>(i - 1)] : 0.0;
            c[static_cast<size_t>(i)] = alpha * c[static_cast<size_t>(i)] + (1.0 - alpha) * cprev;
        }
    }

    return c[static_cast<size_t>(J)];
}

// Divided-differences method.
// B_i^d(t) = (-1)^(d+1) * (d+1) * [t_i,...,t_{i+d+1}](t-x)^d_+
// With uniform knots the d+2 nodes are {i, i+1, ..., i+d+1} and each
// divided-difference denominator at level k equals k (spacing = 1).
double Project5Component::evalDividedDiff(double t) const
{
    const int nc = numCoeffs();
    if (nc <= 0) return 0.0;

    const int     numNodes = d + 2;                      // t_i ... t_{i+d+1}
    const double  sign     = ((d + 1) % 2 == 0) ? 1.0 : -1.0;
    const double  scale    = sign * static_cast<double>(d + 1);

    std::vector<double> g(static_cast<size_t>(numNodes));
    double result = 0.0;

    for (int i = 0; i < nc; ++i)
    {
        // Initialise: g[j] = (t - (i+j))^d_+
        for (int j = 0; j < numNodes; ++j)
        {
            const double diff = t - static_cast<double>(i + j);
            g[static_cast<size_t>(j)] = (diff > 0.0) ? std::pow(diff, static_cast<double>(d)) : 0.0;
        }

        // Build divided-difference table in-place (denominator = k at level k)
        for (int k = 1; k < numNodes; ++k)
        {
            for (int j = numNodes - 1; j >= k; --j)
                g[static_cast<size_t>(j)] = (g[static_cast<size_t>(j)] - g[static_cast<size_t>(j - 1)])
                                            / static_cast<double>(k);
        }

        result += coeffs[static_cast<size_t>(i)] * scale * g[static_cast<size_t>(d + 1)];
    }

    return result;
}

double Project5Component::eval(double t) const
{
    return (method == EvalMethod::DeBoor) ? evalDeBoor(t) : evalDividedDiff(t);
}

// ============================================================================
// Coordinate helpers
// ============================================================================

juce::Rectangle<float> Project5Component::getPlotRect() const
{
    auto b = getLocalBounds().toFloat();
    b.removeFromTop(static_cast<float>(kMargin + kToolH + kMargin));
    return b.withTrimmedLeft(kPadL)
             .withTrimmedRight(kPadR)
             .withTrimmedTop(kPadT)
             .withTrimmedBottom(kPadB);
}

juce::Point<float> Project5Component::toScreen(double xW, double yW, juce::Rectangle<float> plot) const
{
    const float sx = static_cast<float>(juce::jmap(xW, 0.0, xWorldMax(),
        static_cast<double>(plot.getX()), static_cast<double>(plot.getRight())));
    const float sy = static_cast<float>(juce::jmap(yW, kYMin, kYMax,
        static_cast<double>(plot.getBottom()), static_cast<double>(plot.getY())));
    return { sx, sy };
}

double Project5Component::toWorldY(float ys, juce::Rectangle<float> plot) const
{
    return juce::jmap(static_cast<double>(ys),
        static_cast<double>(plot.getBottom()), static_cast<double>(plot.getY()),
        kYMin, kYMax);
}

int Project5Component::pickDot(juce::Point<float> pos, juce::Rectangle<float> plot) const
{
    constexpr float kHitR2 = 100.0f; // 10px radius
    const int nc = numCoeffs();
    for (int i = 0; i < nc; ++i)
    {
        const auto p = toScreen(dotXWorld(i), coeffs[static_cast<size_t>(i)], plot);
        if (p.getDistanceSquaredFrom(pos) <= kHitR2)
            return i;
    }
    return -1;
}

// ============================================================================
// Drawing
// ============================================================================

void Project5Component::drawBackground(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    if (N - d <= d) return;
    const auto p0 = toScreen(static_cast<double>(d),     kYMin, plot);
    const auto p1 = toScreen(static_cast<double>(N - d), kYMax, plot);
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.fillRect(juce::Rectangle<float>(p0.x, p1.y, p1.x - p0.x, p0.y - p1.y));
}

void Project5Component::drawGrid(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    // Vertical lines at integer knots
    g.setColour(juce::Colours::white.withAlpha(0.07f));
    for (int k = 0; k <= N; ++k)
    {
        const auto p0 = toScreen(static_cast<double>(k), kYMin, plot);
        const auto p1 = toScreen(static_cast<double>(k), kYMax, plot);
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.0f);
    }

    // Horizontal lines at integer y values
    for (int y = static_cast<int>(kYMin); y <= static_cast<int>(kYMax); ++y)
    {
        const auto p0 = toScreen(0.0,        static_cast<double>(y), plot);
        const auto p1 = toScreen(xWorldMax(), static_cast<double>(y), plot);
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.0f);
    }

    // Brighter y = 0 axis
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    {
        const auto p0 = toScreen(0.0,        0.0, plot);
        const auto p1 = toScreen(xWorldMax(), 0.0, plot);
        g.drawLine(p0.x, p0.y, p1.x, p1.y, 1.5f);
    }

    // Border
    g.setColour(juce::Colours::grey.withAlpha(0.5f));
    g.drawRect(plot, 1.0f);
}

void Project5Component::drawReferenceLine(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    if (N - d <= d) return;   // no active interval
    // y = 1 line over [d, N-d]: initial state (partition-of-unity)
    const auto p0 = toScreen(static_cast<double>(d),     1.0, plot);
    const auto p1 = toScreen(static_cast<double>(N - d), 1.0, plot);
    g.setColour(juce::Colours::yellow.withAlpha(0.35f));
    g.drawLine(p0.x, p0.y, p1.x, p1.y, 2.0f);
}

void Project5Component::drawCurve(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    if (numCoeffs() <= 0 || N - d <= d) return;

    const double tStart  = static_cast<double>(d);
    const double tEnd    = static_cast<double>(N - d);
    constexpr int kSamples = 800;

    juce::Path path;
    for (int s = 0; s <= kSamples; ++s)
    {
        const double t = tStart + (tEnd - tStart) * static_cast<double>(s) / kSamples;
        double y = eval(t);
        y = juce::jlimit(kYMin, kYMax, y);
        const auto p = toScreen(t, y, plot);
        if (s == 0) path.startNewSubPath(p);
        else        path.lineTo(p);
    }

    g.setColour(juce::Colours::white);
    g.strokePath(path, juce::PathStrokeType(2.5f));
}

void Project5Component::drawDots(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    constexpr float kRad = 6.0f;
    constexpr float kStr = 2.0f;
    const int nc = numCoeffs();

    for (int i = 0; i < nc; ++i)
    {
        const auto p = toScreen(dotXWorld(i), coeffs[static_cast<size_t>(i)], plot);
        g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
        g.drawEllipse(p.x - kRad, p.y - kRad, kRad * 2.0f, kRad * 2.0f, kStr);
    }
}

void Project5Component::drawAxisLabels(juce::Graphics& g, juce::Rectangle<float> plot) const
{
    g.setFont(12.0f);

    // Adaptive step so we get ~10–15 x-axis labels regardless of N
    const int step = (N <= 15) ? 1 : (N <= 30) ? 2 : 5;

    // x-axis knot labels (below plot)
    g.setColour(juce::Colours::lightgrey);
    for (int k = 0; k <= N; k += step)
    {
        const auto p = toScreen(static_cast<double>(k), kYMin, plot);
        g.drawText(juce::String(k),
                   static_cast<int>(p.x) - 12, static_cast<int>(p.y) + 4,
                   24, 14, juce::Justification::centred);
    }

    // y-axis labels (left of plot)
    for (int y = static_cast<int>(kYMin); y <= static_cast<int>(kYMax); ++y)
    {
        const auto p = toScreen(0.0, static_cast<double>(y), plot);
        g.drawText(juce::String(y),
                   static_cast<int>(plot.getX()) - 44, static_cast<int>(p.y) - 7,
                   38, 14, juce::Justification::right);
    }

    // Active-interval boundary markers
    if (N - d > d)
    {
        g.setColour(juce::Colours::yellow.withAlpha(0.7f));
        g.setFont(11.0f);
        {
            const auto p = toScreen(static_cast<double>(d), kYMax, plot);
            g.drawText("t=" + juce::String(d),
                       static_cast<int>(p.x) - 18, static_cast<int>(p.y) - 20,
                       36, 14, juce::Justification::centred);
        }
        {
            const auto p = toScreen(static_cast<double>(N - d), kYMax, plot);
            g.drawText("t=" + juce::String(N - d),
                       static_cast<int>(p.x) - 18, static_cast<int>(p.y) - 20,
                       36, 14, juce::Justification::centred);
        }
    }
}

// ============================================================================
// JUCE Component overrides
// ============================================================================

void Project5Component::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkslategrey);

    const auto plot = getPlotRect();
    drawBackground(g, plot);
    drawGrid(g, plot);
    drawReferenceLine(g, plot);
    drawCurve(g, plot);
    drawDots(g, plot);
    drawAxisLabels(g, plot);
}

void Project5Component::resized()
{
    auto area = getLocalBounds().reduced(kMargin);
    auto top  = area.removeFromTop(kToolH);

    degreeLabel.setBounds(top.removeFromLeft(55));
    degreeSlider.setBounds(top.removeFromLeft(170));
    top.removeFromLeft(12);
    nLabel.setBounds(top.removeFromLeft(22));
    nSlider.setBounds(top.removeFromLeft(170));
    top.removeFromLeft(12);
    methodLabel.setBounds(top.removeFromLeft(55));
    methodBox.setBounds(top.removeFromLeft(170));
    top.removeFromLeft(12);
    resetButton.setBounds(top.removeFromLeft(70));
}

void Project5Component::mouseDown(const juce::MouseEvent& e)
{
    dragIndex = pickDot(e.position, getPlotRect());
}

void Project5Component::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex < 0) return;
    auto plot = getPlotRect();
    double yW = toWorldY(e.position.y, plot);
    yW = juce::jlimit(kYMin, kYMax, yW);
    coeffs[static_cast<size_t>(dragIndex)] = yW;
    repaint();
}

void Project5Component::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}
