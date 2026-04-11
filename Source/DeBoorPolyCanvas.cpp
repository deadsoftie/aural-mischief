#include "DeBoorPolyCanvas.h"

DeBoorPolyCanvas::DeBoorPolyCanvas()
{
    buildClampedKnots();
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void DeBoorPolyCanvas::setDegree(int newDegree)
{
    degree = juce::jlimit(1, 20, newDegree);
    deBoorModeActive = false;
    deBoorPts.clear();
    dragIndex = -1;

    // clamp Bezier points to new degree
    const int maxPts = degree + 1;
    if (static_cast<int>(bezierPts.size()) > maxPts)
        bezierPts.resize(static_cast<size_t>(maxPts));

    buildClampedKnots();
    repaint();
}

void DeBoorPolyCanvas::clearPoints()
{
    bezierPts.clear();
    deBoorPts.clear();
    deBoorModeActive = false;
    dragIndex = -1;
    buildClampedKnots();
    repaint();
}

void DeBoorPolyCanvas::switchKnots()
{
    if (!deBoorModeActive)
    {
        if (static_cast<int>(bezierPts.size()) < 2)
            return;

        buildSpreadKnots(static_cast<int>(bezierPts.size()) - 1);
        computeDeBoorPointsFromPolarForm();
        deBoorModeActive = true;
    }
    else
    {
        // Reconstruct Bezier control points from the current De Boor points so
        // the curve is continuous across the mode switch.
        computeBezierFromDeBoor();
        deBoorModeActive = false;
        deBoorPts.clear();
        buildClampedKnots();
    }

    dragIndex = -1;
    repaint();
}

juce::String DeBoorPolyCanvas::getKnotString() const
{
    // In DeBoor mode the stored spread knots are always correct.
    // In Bezier/clamped mode, build clamped knots for the effective degree
    // (number of points currently placed minus 1) so the display matches
    // what is actually rendered.
    std::vector<double> display;

    if (deBoorModeActive)
    {
        display = knots;
    }
    else
    {
        const int effDeg = juce::jmax(0, static_cast<int>(bezierPts.size()) - 1);
        const int total  = 2 * effDeg + 2;
        display.assign(static_cast<size_t>(total), 0.0);
        for (int i = effDeg + 1; i < total; ++i)
            display[static_cast<size_t>(i)] = 1.0;
    }

    juce::String s;
    for (int i = 0; i < static_cast<int>(display.size()); ++i)
    {
        if (i > 0) s += ", ";
        s += juce::String(display[static_cast<size_t>(i)], 3);
    }
    return s;
}

// ─────────────────────────────────────────────────────────────────────────────
// Knot management
// ─────────────────────────────────────────────────────────────────────────────

void DeBoorPolyCanvas::buildClampedKnots()
{
    // [0,...,0, 1,...,1]  (d+1 zeros, d+1 ones, total 2d+2 values)
    const int total = 2 * degree + 2;
    knots.assign(static_cast<size_t>(total), 0.0);
    for (int i = degree + 1; i < total; ++i)
        knots[static_cast<size_t>(i)] = 1.0;
}

void DeBoorPolyCanvas::buildSpreadKnots(int d)
{
    const int N = 2 * d + 1;
    knots.resize(static_cast<size_t>(N) + 1);

    for (int i = 0; i <= d; ++i)
        knots[static_cast<size_t>(i)] = (i - d) / 10.0;

    knots[static_cast<size_t>(d + 1)] = 1.0;

    for (int i = d + 2; i <= N; ++i)
        knots[static_cast<size_t>(i)] = 1.0 + (i - d - 1) / 10.0;
}

// ─────────────────────────────────────────────────────────────────────────────
// Polar form and DeBoor point computation
// ─────────────────────────────────────────────────────────────────────────────

juce::Point<float> DeBoorPolyCanvas::polarForm(const std::vector<double>& args) const
{
    // F[u1,...,ud] via NLI: apply one linear blend per argument.
    // args.size() must equal bezierPts.size() - 1 (the effective degree).
    std::vector<juce::Point<float>> pts = bezierPts;

    for (double u : args)
    {
        const int n = static_cast<int>(pts.size()) - 1;
        for (int i = 0; i < n; ++i)
            pts[static_cast<size_t>(i)] = pts[static_cast<size_t>(i)] * static_cast<float>(1.0 - u)
                                        + pts[static_cast<size_t>(i) + 1] * static_cast<float>(u);
        pts.resize(static_cast<size_t>(n));
    }

    return pts.empty() ? juce::Point<float>{} : pts[0];
}

void DeBoorPolyCanvas::computeDeBoorPointsFromPolarForm()
{
    // Q[i] = F[t_{i+1}, ..., t_{i+d}]  for i = 0,...,d
    // d here is the effective degree (bezierPts.size() - 1).
    const int d = static_cast<int>(bezierPts.size()) - 1;
    deBoorPts.resize(static_cast<size_t>(d) + 1);

    for (int i = 0; i <= d; ++i)
    {
        std::vector<double> args;
        args.reserve(static_cast<size_t>(d));
        for (int j = 1; j <= d; ++j)
            args.push_back(knots[static_cast<size_t>(i + j)]);
        deBoorPts[static_cast<size_t>(i)] = polarForm(args);
    }
}

void DeBoorPolyCanvas::computeBezierFromDeBoor()
{
    // Reconstruct Bezier control points P[0..d] from the current De Boor points
    // Q[0..d] using the multi-parameter blossom:
    //
    //   P_j = F_Q [ 0^(d-j),  1^j ]
    //
    // i.e. run the De Boor recurrence for d stages, using u=0 for the first
    // (d-j) stages and u=1 for the remaining j stages.  This is verified to
    // invert the polar-form computation exactly.

    const int d = static_cast<int>(deBoorPts.size()) - 1;
    bezierPts.resize(static_cast<size_t>(d) + 1);

    for (int j = 0; j <= d; ++j)
    {
        std::vector<juce::Point<float>> q = deBoorPts;

        for (int p = 1; p <= d; ++p)
        {
            // First (d-j) stages use u=0; last j stages use u=1.
            const double u = (p <= d - j) ? 0.0 : 1.0;

            for (int i = d; i >= p; --i)
            {
                const double ki    = knots[static_cast<size_t>(i)];
                const double kiEnd = knots[static_cast<size_t>(i + d - (p - 1))];
                const double denom = kiEnd - ki;

                const double alpha = (std::abs(denom) < 1e-12) ? 0.0 : (u - ki) / denom;
                q[static_cast<size_t>(i)] =
                    q[static_cast<size_t>(i)]     * static_cast<float>(alpha)
                  + q[static_cast<size_t>(i) - 1] * static_cast<float>(1.0 - alpha);
            }
        }

        bezierPts[static_cast<size_t>(j)] = q[static_cast<size_t>(d)];
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Curve evaluation
// ─────────────────────────────────────────────────────────────────────────────

juce::Point<float> DeBoorPolyCanvas::evalNLI(double tt) const
{
    const int n = static_cast<int>(bezierPts.size());
    if (n == 0) return {};
    if (n == 1) return bezierPts[0];

    // In-place De Casteljau: work on a copy, reduce by one point per stage.
    std::vector<juce::Point<float>> pts = bezierPts;
    for (int r = 1; r < n; ++r)
        for (int i = 0; i < n - r; ++i)
            pts[static_cast<size_t>(i)] =
                pts[static_cast<size_t>(i)]     * static_cast<float>(1.0 - tt)
              + pts[static_cast<size_t>(i) + 1] * static_cast<float>(tt);

    return pts[0];
}

juce::Point<float> DeBoorPolyCanvas::evalDeBoor(double tt) const
{
    const int n = static_cast<int>(deBoorPts.size());
    if (n == 0) return {};
    if (n == 1) return deBoorPts[0];

    const int d = n - 1;
    std::vector<juce::Point<float>> q = deBoorPts;

    for (int p = 1; p <= d; ++p)
    {
        // Process i descending so the in-place update reads unmodified values.
        for (int i = d; i >= p; --i)
        {
            const double ki    = knots[static_cast<size_t>(i)];
            const double kiEnd = knots[static_cast<size_t>(i + d - (p - 1))];
            const double denom = kiEnd - ki;

            const double alpha = (std::abs(denom) < 1e-12) ? 0.0 : (tt - ki) / denom;
            q[static_cast<size_t>(i)] =
                q[static_cast<size_t>(i)]     * static_cast<float>(alpha)
              + q[static_cast<size_t>(i) - 1] * static_cast<float>(1.0 - alpha);
        }
    }

    return q[static_cast<size_t>(d)];
}

// ─────────────────────────────────────────────────────────────────────────────
// Picking
// ─────────────────────────────────────────────────────────────────────────────

int DeBoorPolyCanvas::pickPoint(juce::Point<float> pos) const
{
    constexpr float r2 = kHitR * kHitR;
    const auto& pts = deBoorModeActive ? deBoorPts : bezierPts;

    for (int i = 0; i < static_cast<int>(pts.size()); ++i)
        if (pts[static_cast<size_t>(i)].getDistanceSquaredFrom(pos) <= r2)
            return i;
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse
// ─────────────────────────────────────────────────────────────────────────────

void DeBoorPolyCanvas::mouseDown(const juce::MouseEvent& e)
{
    dragIndex = pickPoint(e.position);

    if (dragIndex < 0)
    {
        if (!deBoorModeActive)
        {
            if (static_cast<int>(bezierPts.size()) < degree + 1)
            {
                bezierPts.emplace_back(e.position);
                if (onKnotsChanged) onKnotsChanged();
                repaint();
            }
        }
        else
        {
            if (static_cast<int>(deBoorPts.size()) < degree + 1)
            {
                deBoorPts.emplace_back(e.position);
                buildSpreadKnots(static_cast<int>(deBoorPts.size()) - 1);
                if (onKnotsChanged) onKnotsChanged();
                repaint();
            }
        }
    }
}

void DeBoorPolyCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex < 0) return;

    if (deBoorModeActive)
        deBoorPts[static_cast<size_t>(dragIndex)] = e.position;
    else
        bezierPts[static_cast<size_t>(dragIndex)] = e.position;

    repaint();
}

void DeBoorPolyCanvas::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Drawing helpers
// ─────────────────────────────────────────────────────────────────────────────

void DeBoorPolyCanvas::drawPolyline(juce::Graphics& g,
    const std::vector<juce::Point<float>>& pts,
    juce::Colour colour, float thickness, bool dashed) const
{
    if (pts.size() < 2) return;
    juce::Path path;
    path.startNewSubPath(pts[0]);
    for (size_t i = 1; i < pts.size(); ++i) path.lineTo(pts[i]);

    if (dashed)
    {
        const float dashLengths[] = { 6.0f, 4.0f };
        juce::PathStrokeType pst(thickness);
        juce::Path dashed_path;
        pst.createDashedStroke(dashed_path, path, dashLengths, 2);
        g.setColour(colour);
        g.fillPath(dashed_path);
    }
    else
    {
        g.setColour(colour);
        g.strokePath(path, juce::PathStrokeType(thickness));
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────

void DeBoorPolyCanvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    const bool haveFull = (static_cast<int>(bezierPts.size()) == degree + 1);

    // ── Bezier polyline ───────────────────────────────────────────────────────
    if (bezierPts.size() >= 2)
    {
        const auto col = deBoorModeActive
            ? juce::Colours::grey.withAlpha(0.35f)
            : juce::Colours::grey.brighter(0.3f);
        drawPolyline(g, bezierPts, col, deBoorModeActive ? 1.0f : 1.5f, deBoorModeActive);
    }

    // ── Bezier curve + NLI shells (Bezier mode only) ──────────────────────────
    if (!deBoorModeActive && static_cast<int>(bezierPts.size()) >= 2)
    {
        // Curve
        constexpr int samples = 600;
        juce::Path curve;
        curve.startNewSubPath(evalNLI(0.0));
        for (int s = 1; s <= samples; ++s)
            curve.lineTo(evalNLI(static_cast<double>(s) / samples));
        g.setColour(juce::Colours::deepskyblue);
        g.strokePath(curve, juce::PathStrokeType(2.5f));
    }

    // ── DeBoor mode: curve + shells ───────────────────────────────────────────
    if (deBoorModeActive && static_cast<int>(deBoorPts.size()) >= 2)
    {
        // DeBoor polyline (orange)
        drawPolyline(g, deBoorPts, juce::Colours::orange, 1.5f);

        // DeBoor curve (green)
        constexpr int samples = 600;
        juce::Path curve;
        curve.startNewSubPath(evalDeBoor(0.0));
        for (int s = 1; s <= samples; ++s)
            curve.lineTo(evalDeBoor(static_cast<double>(s) / samples));
        g.setColour(juce::Colours::limegreen);
        g.strokePath(curve, juce::PathStrokeType(2.5f));
    }

    // ── Bezier control points ─────────────────────────────────────────────────
    g.setFont(11.0f);
    for (int i = 0; i < static_cast<int>(bezierPts.size()); ++i)
    {
        const auto p = bezierPts[static_cast<size_t>(i)];
        const float r  = deBoorModeActive ? 4.0f : 6.0f;
        const float alpha = deBoorModeActive ? 0.35f : 1.0f;

        g.setColour((i == dragIndex && !deBoorModeActive)
            ? juce::Colours::yellow
            : juce::Colours::cyan.withAlpha(alpha));
        g.fillEllipse(p.x - r, p.y - r, r * 2.0f, r * 2.0f);

        if (!deBoorModeActive)
        {
            g.setColour(juce::Colours::white.withAlpha(0.75f));
            g.drawText("P" + juce::String(i),
                static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 8, 30, 16,
                juce::Justification::left);
        }
    }

    // ── DeBoor control points ─────────────────────────────────────────────────
    if (deBoorModeActive)
    {
        for (int i = 0; i < static_cast<int>(deBoorPts.size()); ++i)
        {
            const auto p = deBoorPts[static_cast<size_t>(i)];
            g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::orange);
            g.fillEllipse(p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);
            g.setColour(juce::Colours::white);
            g.drawText("Q" + juce::String(i),
                static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 8, 30, 16,
                juce::Justification::left);
        }
    }

    // ── Instructions ──────────────────────────────────────────────────────────
    if (!deBoorModeActive && !haveFull)
    {
        g.setColour(juce::Colours::white.withAlpha(0.65f));
        g.setFont(15.0f);
        g.drawText("Click to place control points  ("
            + juce::String(bezierPts.size()) + " / " + juce::String(degree + 1) + ")",
            getLocalBounds().reduced(12), juce::Justification::topLeft);
    }
}
