#include "BSplineCanvas.h"

BSplineCanvas::BSplineCanvas() {}

void BSplineCanvas::setDegree(int d)
{
    degree = juce::jlimit(1, 10, d);
    buildDefaultKnots();
    if (onStateChanged) onStateChanged();
    repaint();
}

void BSplineCanvas::setT(double t)
{
    tParam = t;
    repaint();
}

void BSplineCanvas::clearPoints()
{
    controlPts.clear();
    dragIndex = -1;
    buildDefaultKnots();
    if (onStateChanged) onStateChanged();
    repaint();
}

void BSplineCanvas::applyClampedPreset()
{
    int s = static_cast<int>(controlPts.size()) - 1;
    int d = degree;
    if (s < d + 1) return;   // need at least d+2 points

    int k = s - d;
    knots.clear();

    for (int i = 0; i <= d; ++i)
        knots.push_back(0.0);

    for (int i = 1; i <= k - 1; ++i)
        knots.push_back(static_cast<double>(i));

    for (int i = 0; i <= d; ++i)
        knots.push_back(static_cast<double>(k));

    tParam = 0.0;
    if (onStateChanged) onStateChanged();
    repaint();
}

void BSplineCanvas::setKnotsFromString(const juce::String& str)
{
    auto tokens = juce::StringArray::fromTokens(str, ",", "");
    std::vector<double> parsed;
    for (auto& tok : tokens)
        parsed.push_back(tok.trim().getDoubleValue());

    // Ensure non-decreasing
    for (int i = 1; i < static_cast<int>(parsed.size()); ++i)
        if (parsed[static_cast<size_t>(i)] < parsed[static_cast<size_t>(i - 1)])
            parsed[static_cast<size_t>(i)] = parsed[static_cast<size_t>(i - 1)];

    int required = static_cast<int>(controlPts.size()) + degree + 1;

    // Extend with consecutive integers if too short
    while (static_cast<int>(parsed.size()) < required)
    {
        double next = parsed.empty() ? 0.0 : std::ceil(parsed.back()) + 1.0;
        parsed.push_back(next);
    }

    // Trim if too long
    parsed.resize(static_cast<size_t>(required));

    knots = parsed;
    tParam = getTMin();
    if (onStateChanged) onStateChanged();
    repaint();
}

juce::String BSplineCanvas::getKnotString() const
{
    juce::String s;
    for (int i = 0; i < static_cast<int>(knots.size()); ++i)
    {
        if (i > 0) s += ", ";
        double v = knots[static_cast<size_t>(i)];
        if (std::abs(v - std::round(v)) < 1e-9)
            s += juce::String(static_cast<int>(std::round(v)));
        else
            s += juce::String(v, 3);
    }
    return s;
}

double BSplineCanvas::getTMin() const
{
    return knots.size() > static_cast<size_t>(degree) ? knots[static_cast<size_t>(degree)] : 0.0;
}

double BSplineCanvas::getTMax() const
{
    int N = static_cast<int>(knots.size()) - 1;
    return (N - degree >= 0) ? knots[static_cast<size_t>(N - degree)] : 0.0;
}

bool BSplineCanvas::hasValidCurve() const
{
    return static_cast<int>(controlPts.size()) > degree
        && static_cast<int>(knots.size()) == static_cast<int>(controlPts.size()) + degree + 1;
}

void BSplineCanvas::buildDefaultKnots()
{
    int s = static_cast<int>(controlPts.size()) - 1;
    int N = s + degree + 1;
    knots.resize(static_cast<size_t>(N) + 1);
    for (int i = 0; i <= N; ++i)
        knots[static_cast<size_t>(i)] = static_cast<double>(i);
}

int BSplineCanvas::findSpan(double t) const
{
    int N  = static_cast<int>(knots.size()) - 1;
    int hi = N - degree;
    if (t >= knots[static_cast<size_t>(hi)]) return hi - 1;
    int lo = degree;
    while (hi - lo > 1)
    {
        int mid = (lo + hi) / 2;
        if (t < knots[static_cast<size_t>(mid)]) hi = mid;
        else lo = mid;
    }
    return lo;
}

std::vector<std::vector<juce::Point<float>>> BSplineCanvas::computeShells(double t) const
{
    if (!hasValidCurve()) return {};
    int J = findSpan(t);
    int d = degree;

    std::vector<std::vector<juce::Point<float>>> shells(static_cast<size_t>(d) + 1);

    // Stage 0: active control points
    shells[0].resize(static_cast<size_t>(d) + 1);
    for (int i = 0; i <= d; ++i)
        shells[0][static_cast<size_t>(i)] = controlPts[static_cast<size_t>(J - d + i)];

    // Stages 1..d
    for (int p = 1; p <= d; ++p)
    {
        int count = d - p + 1;
        shells[static_cast<size_t>(p)].resize(static_cast<size_t>(count));
        for (int j = 0; j < count; ++j)
        {
            int    gi    = J - d + p + j;
            double denom = knots[static_cast<size_t>(gi + d - (p - 1))] - knots[static_cast<size_t>(gi)];
            double alpha = (std::abs(denom) < 1e-12) ? 0.0 : (t - knots[static_cast<size_t>(gi)]) / denom;
            shells[static_cast<size_t>(p)][static_cast<size_t>(j)] =
                shells[static_cast<size_t>(p - 1)][static_cast<size_t>(j)]     * static_cast<float>(1.0 - alpha)
              + shells[static_cast<size_t>(p - 1)][static_cast<size_t>(j) + 1] * static_cast<float>(alpha);
        }
    }
    return shells;
}

juce::Point<float> BSplineCanvas::evalBSpline(double t) const
{
    auto shells = computeShells(t);
    if (shells.empty() || shells.back().empty()) return {};
    return shells.back()[0];
}

int BSplineCanvas::pickPoint(juce::Point<float> pos) const
{
    for (int i = 0; i < static_cast<int>(controlPts.size()); ++i)
        if (controlPts[static_cast<size_t>(i)].getDistanceSquaredFrom(pos) <= kHitR * kHitR)
            return i;
    return -1;
}

void BSplineCanvas::drawPolyline(juce::Graphics& g,
    const std::vector<juce::Point<float>>& pts,
    juce::Colour col, float thickness) const
{
    if (pts.size() < 2) return;
    juce::Path path;
    path.startNewSubPath(pts[0]);
    for (size_t i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);
    g.setColour(col);
    g.strokePath(path, juce::PathStrokeType(thickness));
}

void BSplineCanvas::mouseDown(const juce::MouseEvent& e)
{
    dragIndex = pickPoint(e.position);
    if (dragIndex < 0)
    {
        controlPts.emplace_back(e.position);
        buildDefaultKnots();
        tParam = getTMin();
        if (onStateChanged) onStateChanged();
        repaint();
    }
}

void BSplineCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex < 0) return;
    controlPts[static_cast<size_t>(dragIndex)] = e.position;
    repaint();
}

void BSplineCanvas::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}

void BSplineCanvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    // ── Control polyline ──────────────────────────────────────────────────────
    drawPolyline(g, controlPts, juce::Colours::grey.brighter(0.3f), 1.5f);

    // ── B-spline curve + shells ───────────────────────────────────────────────
    if (hasValidCurve())
    {
        double lo = getTMin();
        double hi = getTMax();
        constexpr int samples = 600;
        juce::Path curve;
        curve.startNewSubPath(evalBSpline(lo));
        for (int s = 1; s <= samples; ++s)
            curve.lineTo(evalBSpline(lo + (hi - lo) * static_cast<double>(s) / samples));
        g.setColour(juce::Colours::deepskyblue);
        g.strokePath(curve, juce::PathStrokeType(2.5f));

        // ── Shells at tParam ──────────────────────────────────────────────────
        double t = juce::jlimit(lo, hi, tParam);
        auto   shells = computeShells(t);

        const juce::Colour shellColours[] = {
            juce::Colours::orange.withAlpha(0.6f),
            juce::Colours::yellow.withAlpha(0.6f),
            juce::Colours::limegreen.withAlpha(0.6f),
            juce::Colours::hotpink.withAlpha(0.6f),
        };
        const int nColours = static_cast<int>(std::size(shellColours));

        for (int p = 0; p < static_cast<int>(shells.size()) - 1; ++p)
        {
            juce::Colour col = shellColours[static_cast<size_t>(p) % static_cast<size_t>(nColours)];
            drawPolyline(g, shells[static_cast<size_t>(p)], col, 1.2f);
            g.setColour(col.brighter(0.3f));
            for (auto& pt : shells[static_cast<size_t>(p)])
                g.fillEllipse(pt.x - 4.0f, pt.y - 4.0f, 8.0f, 8.0f);
        }

        // Highlight the point on the curve
        if (!shells.empty() && !shells.back().empty())
        {
            auto pt = shells.back()[0];
            g.setColour(juce::Colours::white);
            g.fillEllipse(pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);
            g.setColour(juce::Colours::deepskyblue);
            g.fillEllipse(pt.x - 4.0f, pt.y - 4.0f, 8.0f, 8.0f);
        }
    }

    // ── Control points ────────────────────────────────────────────────────────
    g.setFont(11.0f);
    for (int i = 0; i < static_cast<int>(controlPts.size()); ++i)
    {
        auto p = controlPts[static_cast<size_t>(i)];
        g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
        g.fillEllipse(p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::white.withAlpha(0.75f));
        g.drawText("P" + juce::String(i),
            static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 8, 30, 16,
            juce::Justification::left);
    }

    // ── Instructions ─────────────────────────────────────────────────────────
    if (static_cast<int>(controlPts.size()) <= degree)
    {
        g.setColour(juce::Colours::white.withAlpha(0.6f));
        g.setFont(15.0f);
        g.drawText("Click to place control points  ("
            + juce::String(static_cast<int>(controlPts.size())) + " placed, need > " + juce::String(degree) + ")",
            getLocalBounds().reduced(12), juce::Justification::topLeft);
    }
}
