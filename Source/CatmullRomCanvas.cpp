#include "CatmullRomCanvas.h"

CatmullRomCanvas::CatmullRomCanvas()
{
    setWantsKeyboardFocus(false);
}

void CatmullRomCanvas::setParameterization(Parameterization p)
{
    param = p;
    repaint();
}

void CatmullRomCanvas::setShowTangents(bool show)
{
    showTangents = show;
    repaint();
}

void CatmullRomCanvas::setShowBezierEquivalent(bool show)
{
    showBezierEquiv = show;
    repaint();
}

void CatmullRomCanvas::setExtendToEndpoints(bool extend)
{
    extendToEndpoints = extend;
    repaint();
}

void CatmullRomCanvas::clearPoints()
{
    pts.clear();
    dragIndex = -1;
    repaint();
    if (onPointsChanged) onPointsChanged();
}

int CatmullRomCanvas::getSegmentCount() const
{
    const int n = static_cast<int>(pts.size());
    if (n < 4) return 0;
    if (extendToEndpoints) return n - 1;
    return n - 3;
}

int CatmullRomCanvas::pickPoint(juce::Point<float> pos) const
{
    const float r2 = hitRadius * hitRadius;
    for (int i = 0; i < static_cast<int>(pts.size()); ++i)
        if (pts[static_cast<size_t>(i)].getDistanceSquaredFrom(pos) <= r2)
            return i;
    return -1;
}

double CatmullRomCanvas::getAlpha() const
{
    switch (param)
    {
        case Parameterization::Uniform:     return 0.0;
        case Parameterization::Centripetal: return 0.5;
        case Parameterization::Chordal:     return 1.0;
    }
    return 0.5;
}

std::vector<double> CatmullRomCanvas::buildKnots(
    const std::vector<juce::Point<float>>& p, double alpha)
{
    const int n = static_cast<int>(p.size());
    std::vector<double> t(static_cast<size_t>(n));
    t[0] = 0.0;
    for (int i = 1; i < n; ++i)
    {
        const double dx = static_cast<double>(p[static_cast<size_t>(i)].x - p[static_cast<size_t>(i - 1)].x);
        const double dy = static_cast<double>(p[static_cast<size_t>(i)].y - p[static_cast<size_t>(i - 1)].y);
        const double dist = std::sqrt(dx * dx + dy * dy);
        // For uniform alpha=0: pow(dist,0)=1 each step, but we still need equal spacing.
        // Guard against coincident points.
        double step = (alpha == 0.0) ? 1.0 : std::pow(std::max(dist, 1e-10), alpha);
        t[static_cast<size_t>(i)] = t[static_cast<size_t>(i - 1)] + step;
    }
    return t;
}

juce::Point<float> CatmullRomCanvas::evalBarryGoldman(
    juce::Point<float> P0, juce::Point<float> P1,
    juce::Point<float> P2, juce::Point<float> P3,
    double t0, double t1, double t2, double t3,
    double u)
{
    // Guard against degenerate knot intervals
    auto lerp = [](juce::Point<float> a, juce::Point<float> b, double s) -> juce::Point<float>
    {
        return a * static_cast<float>(1.0 - s) + b * static_cast<float>(s);
    };

    auto safeLerp = [&](juce::Point<float> a, juce::Point<float> b,
                        double num, double den) -> juce::Point<float>
    {
        if (std::abs(den) < 1e-12) return a;
        return lerp(a, b, num / den);
    };

    // Round 1: 4 pts -> 3
    const auto A1 = safeLerp(P0, P1, u - t0, t1 - t0);
    const auto A2 = safeLerp(P1, P2, u - t1, t2 - t1);
    const auto A3 = safeLerp(P2, P3, u - t2, t3 - t2);

    // Round 2: 3 pts -> 2
    const auto B1 = safeLerp(A1, A2, u - t0, t2 - t0);
    const auto B2 = safeLerp(A2, A3, u - t1, t3 - t1);

    // Round 3: 2 pts -> 1
    return safeLerp(B1, B2, u - t1, t2 - t1);
}

void CatmullRomCanvas::buildCurve(std::vector<juce::Point<float>>& out) const
{
    out.clear();

    if (pts.size() < 4)
        return;

    const double alpha = getAlpha();
    constexpr int samplesPerSegment = 200;

    if (extendToEndpoints)
    {
        // Reflect pts[0] about pts[1] and pts[n-1] about pts[n-2]
        const int n = static_cast<int>(pts.size());
        std::vector<juce::Point<float>> extended;
        extended.reserve(static_cast<size_t>(n + 2));

        // phantom start: reflect pts[0] about pts[1]
        extended.push_back(pts[1] * 2.0f - pts[0]);
        for (const auto& p : pts)
            extended.push_back(p);
        // phantom end: reflect pts[n-1] about pts[n-2]
        extended.push_back(pts[static_cast<size_t>(n - 2)] * 2.0f - pts[static_cast<size_t>(n - 1)]);

        const auto knots = buildKnots(extended, alpha);
        const int en = static_cast<int>(extended.size());

        // Iterate over segments 1 .. en-3 (inclusive), which maps to original pts[0]..pts[n-1]
        for (int i = 1; i <= en - 3; ++i)
        {
            const auto& P0 = extended[static_cast<size_t>(i - 1)];
            const auto& P1 = extended[static_cast<size_t>(i)];
            const auto& P2 = extended[static_cast<size_t>(i + 1)];
            const auto& P3 = extended[static_cast<size_t>(i + 2)];

            const double tk0 = knots[static_cast<size_t>(i - 1)];
            const double tk1 = knots[static_cast<size_t>(i)];
            const double tk2 = knots[static_cast<size_t>(i + 1)];
            const double tk3 = knots[static_cast<size_t>(i + 2)];

            for (int s = 0; s <= samplesPerSegment; ++s)
            {
                const double frac = static_cast<double>(s) / static_cast<double>(samplesPerSegment);
                const double u = tk1 + frac * (tk2 - tk1);
                out.push_back(evalBarryGoldman(P0, P1, P2, P3, tk0, tk1, tk2, tk3, u));
            }
        }
    }
    else
    {
        const auto knots = buildKnots(pts, alpha);
        const int n = static_cast<int>(pts.size());

        // Segments i = 1 .. n-3 (uses pts[i-1..i+2])
        for (int i = 1; i <= n - 3; ++i)
        {
            const auto& P0 = pts[static_cast<size_t>(i - 1)];
            const auto& P1 = pts[static_cast<size_t>(i)];
            const auto& P2 = pts[static_cast<size_t>(i + 1)];
            const auto& P3 = pts[static_cast<size_t>(i + 2)];

            const double tk0 = knots[static_cast<size_t>(i - 1)];
            const double tk1 = knots[static_cast<size_t>(i)];
            const double tk2 = knots[static_cast<size_t>(i + 1)];
            const double tk3 = knots[static_cast<size_t>(i + 2)];

            for (int s = 0; s <= samplesPerSegment; ++s)
            {
                const double frac = static_cast<double>(s) / static_cast<double>(samplesPerSegment);
                const double u = tk1 + frac * (tk2 - tk1);
                out.push_back(evalBarryGoldman(P0, P1, P2, P3, tk0, tk1, tk2, tk3, u));
            }
        }
    }
}

juce::Point<float> CatmullRomCanvas::computeTangent(
    const std::vector<juce::Point<float>>& p,
    const std::vector<double>& knots,
    int i)
{
    // Centripetal/chordal tangent: (P_{i+1} - P_{i-1}) / (t_{i+1} - t_{i-1})
    const double denom = knots[static_cast<size_t>(i + 1)] - knots[static_cast<size_t>(i - 1)];
    if (std::abs(denom) < 1e-12)
        return {};
    const auto diff = p[static_cast<size_t>(i + 1)] - p[static_cast<size_t>(i - 1)];
    return diff * static_cast<float>(1.0 / denom);
}

void CatmullRomCanvas::drawDashedLine(juce::Graphics& g,
    juce::Point<float> a, juce::Point<float> b,
    juce::Colour colour, float thickness) const
{
    const float dashLengths[] = { 6.0f, 4.0f };
    juce::Path path;
    path.startNewSubPath(a);
    path.lineTo(b);
    g.setColour(colour);
    juce::PathStrokeType stroke(thickness);
    stroke.createDashedStroke(path, path, dashLengths, 2);
    g.strokePath(path, stroke);
}

void CatmullRomCanvas::drawArrow(juce::Graphics& g,
    juce::Point<float> origin, juce::Point<float> tip,
    juce::Colour colour, float thickness) const
{
    g.setColour(colour);
    g.drawLine(origin.x, origin.y, tip.x, tip.y, thickness);

    // Arrowhead
    const auto delta = tip - origin;
    const float dlen = delta.getDistanceFromOrigin();
    if (dlen < 1e-6f) return;
    const auto dir  = delta / dlen;
    const auto perp = juce::Point<float>(-dir.y, dir.x);
    constexpr float headLen = 10.0f;
    constexpr float headWidth = 5.0f;

    const auto left  = tip - dir * headLen + perp * headWidth;
    const auto right = tip - dir * headLen - perp * headWidth;

    juce::Path head;
    head.startNewSubPath(tip);
    head.lineTo(left);
    head.lineTo(right);
    head.closeSubPath();
    g.fillPath(head);
}

void CatmullRomCanvas::paint(juce::Graphics& g)
{
    // 1. Background
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    const int n = static_cast<int>(pts.size());

    // 2. Instruction text
    if (n < 4)
    {
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(16.0f);
        g.drawText("Click to add data points (" + juce::String(n) + "/4 minimum)",
            getLocalBounds().reduced(10), juce::Justification::topLeft);
    }

    // 3. Control polyline (dashed, dim grey)
    if (n >= 2)
    {
        for (int i = 0; i < n - 1; ++i)
            drawDashedLine(g,
                pts[static_cast<size_t>(i)],
                pts[static_cast<size_t>(i + 1)],
                juce::Colours::grey.withAlpha(0.5f), 1.0f);
    }

    if (n >= 4)
    {
        const double alpha = getAlpha();

        // Determine working point set (with phantoms if needed)
        std::vector<juce::Point<float>> workPts;
        std::vector<double> knots;

        if (extendToEndpoints)
        {
            workPts.reserve(static_cast<size_t>(n + 2));
            workPts.push_back(pts[1] * 2.0f - pts[0]);
            for (const auto& p : pts) workPts.push_back(p);
            workPts.push_back(pts[static_cast<size_t>(n - 2)] * 2.0f - pts[static_cast<size_t>(n - 1)]);
            knots = buildKnots(workPts, alpha);
        }
        else
        {
            workPts = pts;
            knots = buildKnots(pts, alpha);
        }

        // 5. Bezier equivalent overlay (faint orange polygons)
        if (showBezierEquiv)
        {
            const int wn = static_cast<int>(workPts.size());
            g.setColour(juce::Colours::orange.withAlpha(0.4f));

            for (int i = 1; i <= wn - 3; ++i)
            {
                const auto& P0 = workPts[static_cast<size_t>(i - 1)];
                const auto& P1 = workPts[static_cast<size_t>(i)];
                const auto& P2 = workPts[static_cast<size_t>(i + 1)];
                const auto& P3 = workPts[static_cast<size_t>(i + 2)];

                const double tk0 = knots[static_cast<size_t>(i - 1)];
                const double tk1 = knots[static_cast<size_t>(i)];
                const double tk2 = knots[static_cast<size_t>(i + 1)];
                const double tk3 = knots[static_cast<size_t>(i + 2)];

                const double dt = tk2 - tk1;
                const double dt20 = tk2 - tk0;
                const double dt31 = tk3 - tk1;

                // Tangent at P1 and P2
                juce::Point<float> tan1, tan2;

                if (std::abs(dt20) > 1e-12)
                    tan1 = (P2 - P0) * static_cast<float>(1.0 / dt20);
                if (std::abs(dt31) > 1e-12)
                    tan2 = (P3 - P1) * static_cast<float>(1.0 / dt31);

                const auto Q0 = P1;
                const auto Q1 = P1 + tan1 * static_cast<float>(dt / 3.0);
                const auto Q2 = P2 - tan2 * static_cast<float>(dt / 3.0);
                const auto Q3 = P2;

                // Draw control polygon
                g.drawLine(Q0.x, Q0.y, Q1.x, Q1.y, 1.0f);
                g.drawLine(Q1.x, Q1.y, Q2.x, Q2.y, 1.0f);
                g.drawLine(Q2.x, Q2.y, Q3.x, Q3.y, 1.0f);

                // Draw inner control points Q1, Q2 as small squares
                constexpr float sq = 5.0f;
                g.fillRect(Q1.x - sq * 0.5f, Q1.y - sq * 0.5f, sq, sq);
                g.fillRect(Q2.x - sq * 0.5f, Q2.y - sq * 0.5f, sq, sq);
            }
        }

        // 4. Catmull-Rom curve (white, 2.5px)
        {
            std::vector<juce::Point<float>> curve;
            buildCurve(curve);

            if (curve.size() >= 2)
            {
                juce::Path path;
                path.startNewSubPath(curve[0]);
                for (size_t i = 1; i < curve.size(); ++i)
                    path.lineTo(curve[i]);

                g.setColour(juce::Colours::white);
                g.strokePath(path, juce::PathStrokeType(2.5f));
            }
        }

        // 6. Tangent arrows at interior points
        if (showTangents)
        {
            // Tangents are computed on the non-phantom pts using pts knots
            const auto ptsKnots = buildKnots(pts, alpha);
            constexpr float arrowLen = 50.0f;

            for (int i = 1; i < n - 1; ++i)
            {
                const auto tan = computeTangent(pts, ptsKnots, i);
                const float len = tan.getDistanceFromOrigin();
                if (len < 1e-6f) continue;

                const auto dir = tan / len;
                const auto origin = pts[static_cast<size_t>(i)];
                const auto tip = origin + dir * arrowLen;

                drawArrow(g, origin, tip, juce::Colours::cyan, 1.5f);
            }
        }
    }

    // 7. Data points
    for (int i = 0; i < n; ++i)
    {
        const auto p = pts[static_cast<size_t>(i)];
        g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
        g.fillEllipse(p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);

        g.setColour(juce::Colours::white.withAlpha(0.8f));
        g.setFont(12.0f);
        g.drawText("P" + juce::String(i),
            static_cast<int>(p.x) + 9, static_cast<int>(p.y) - 8,
            36, 16, juce::Justification::left);
    }
}

void CatmullRomCanvas::mouseDown(const juce::MouseEvent& e)
{
    dragIndex = pickPoint(e.position);
    if (dragIndex < 0)
    {
        pts.emplace_back(e.position);
        repaint();
        if (onPointsChanged) onPointsChanged();
    }
}

void CatmullRomCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex < 0) return;
    pts[static_cast<size_t>(dragIndex)] = e.position;
    repaint();
}

void CatmullRomCanvas::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}
