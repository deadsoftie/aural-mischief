#include "BestFitCanvas.h"
#include <algorithm>
#include <limits>
#include <cmath>

BestFitCanvas::BestFitCanvas()
{
    setOpaque(true);
}

// ─────────────────────────────────────────────────────────────────────────────
// Public interface
// ─────────────────────────────────────────────────────────────────────────────

void BestFitCanvas::setMode(bool parabola)
{
    showParabola = parabola;
    recompute();
}

void BestFitCanvas::clearPoints()
{
    pts.clear();
    hasSubMeans = hasParabola = false;
    lineCost = parabolaCost = 0.0f;
    repaint();
}

// ─────────────────────────────────────────────────────────────────────────────
// Mouse
// ─────────────────────────────────────────────────────────────────────────────

int BestFitCanvas::pickPoint(juce::Point<float> pos) const
{
    for (int i = 0; i < (int)pts.size(); ++i)
        if (pts[i].getDistanceFrom(pos) <= kHitRadius)
            return i;
    return -1;
}

void BestFitCanvas::mouseDown(const juce::MouseEvent& e)
{
    const auto pos = e.position;

    if (e.mods.isRightButtonDown())
    {
        const int idx = pickPoint(pos);
        if (idx >= 0)
        {
            pts.erase(pts.begin() + idx);
            recompute();
        }
        return;
    }

    dragIndex = pickPoint(pos);
    if (dragIndex < 0)
    {
        pts.push_back(pos);
        recompute();
        dragIndex = (int)pts.size() - 1;
    }
}

void BestFitCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex >= 0 && dragIndex < (int)pts.size())
    {
        pts[dragIndex] = e.position;
        recompute();
    }
}

void BestFitCanvas::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}

// ─────────────────────────────────────────────────────────────────────────────
// Fit orchestration
// ─────────────────────────────────────────────────────────────────────────────

void BestFitCanvas::recompute()
{
    hasSubMeans = hasParabola = false;
    lineCost = parabolaCost = 0.0f;

    const int n = (int)pts.size();
    if (n >= 2)
    {
        fitLine();
        if (showParabola && n >= 3)
            fitParabola();
    }
    repaint();
}

// ─────────────────────────────────────────────────────────────────────────────
// Best-fit line
// ─────────────────────────────────────────────────────────────────────────────

void BestFitCanvas::fitLine()
{
    const int n = (int)pts.size();

    // Mean
    M = {0.0f, 0.0f};
    for (auto& p : pts) { M.x += p.x; M.y += p.y; }
    M.x /= (float)n;
    M.y /= (float)n;

    if (n == 2)
    {
        auto d = pts[1] - pts[0];
        const float len = d.getDistanceFromOrigin();
        lineFitDir = (len > 1e-6f) ? d / len : juce::Point<float>{1.0f, 0.0f};
        lineCost   = 0.0f;
        bisDir     = {-lineFitDir.y, lineFitDir.x};
        return;
    }

    // Iterative arc search over [0, pi).  Lines have no orientation, so the
    // half-circle [0, pi) covers all distinct directions.
    float arcStart = 0.0f;
    float arcEnd   = juce::MathConstants<float>::pi;

    struct Cand { float score, angle; };

    for (int iter = 0; iter < kIter; ++iter)
    {
        Cand best[3];
        for (auto& c : best) c = {std::numeric_limits<float>::max(), arcStart};

        for (int i = 0; i < kSamples; ++i)
        {
            const float a = arcStart + (float)i / (float)(kSamples - 1) * (arcEnd - arcStart);
            const float s = scoreLine(a);
            const Cand  c{s, a};
            if      (c.score < best[0].score) { best[2] = best[1]; best[1] = best[0]; best[0] = c; }
            else if (c.score < best[1].score) { best[2] = best[1]; best[1] = c; }
            else if (c.score < best[2].score) { best[2] = c; }
        }

        float angles[3] = {best[0].angle, best[1].angle, best[2].angle};
        std::sort(angles, angles + 3);
        arcStart = angles[0];
        arcEnd   = angles[2];
        if (arcEnd - arcStart < 1e-9f) break;
    }

    // Final pass over the refined arc
    float bestScore = std::numeric_limits<float>::max();
    float bestAngle = arcStart;
    for (int i = 0; i < kSamples; ++i)
    {
        const float a = arcStart + (float)i / (float)(kSamples - 1) * (arcEnd - arcStart);
        const float s = scoreLine(a);
        if (s < bestScore) { bestScore = s; bestAngle = a; }
    }

    lineFitDir = {std::cos(bestAngle), std::sin(bestAngle)};
    lineCost   = bestScore;
    bisDir     = {-lineFitDir.y, lineFitDir.x};
}

float BestFitCanvas::scoreLine(float angle) const noexcept
{
    const juce::Point<float> dir{std::cos(angle), std::sin(angle)};
    float total = 0.0f;
    for (auto& p : pts)
        total += perpDist(p, M, dir);
    return total;
}

// ─────────────────────────────────────────────────────────────────────────────
// Best-fit parabola
// ─────────────────────────────────────────────────────────────────────────────

void BestFitCanvas::fitParabola()
{
    // Candidates from both families; we keep the global best.
    juce::Point<float> bestQ0, bestQ1, bestQ2, bestM1, bestM2;
    float bestCost = std::numeric_limits<float>::max();
    bool  found    = false;

    // tryFamily: partition pts by sign of (p-M)·partitionDir,
    //            then search Q1 along q1AxisDir through M.
    auto tryFamily = [&](juce::Point<float> partitionDir,
                         juce::Point<float> q1AxisDir)
    {
        std::vector<juce::Point<float>> s1, s2;
        for (auto& p : pts)
        {
            const float dot = (p.x - M.x) * partitionDir.x
                            + (p.y - M.y) * partitionDir.y;
            if      (dot < 0.0f) s1.push_back(p);
            else if (dot > 0.0f) s2.push_back(p);
        }
        if (s1.empty() || s2.empty()) return;

        juce::Point<float> m1{0.0f, 0.0f}, m2{0.0f, 0.0f};
        for (auto& p : s1) { m1.x += p.x; m1.y += p.y; }
        m1.x /= (float)s1.size(); m1.y /= (float)s1.size();
        for (auto& p : s2) { m2.x += p.x; m2.y += p.y; }
        m2.x /= (float)s2.size(); m2.y /= (float)s2.size();

        float c = std::max(m1.getDistanceFrom(M), m2.getDistanceFrom(M));
        if (c < 1.0f) c = 1.0f;

        struct Cand { float score, t; };

        float tStart = -4.0f * c;
        float tEnd   =  4.0f * c;

        for (int iter = 0; iter < kIter; ++iter)
        {
            Cand best[3];
            for (auto& b : best) b = {std::numeric_limits<float>::max(), tStart};

            for (int i = 0; i < kSamples; ++i)
            {
                const float t  = tStart + (float)i / (float)(kSamples - 1) * (tEnd - tStart);
                const juce::Point<float> q1{M.x + q1AxisDir.x * t,
                                            M.y + q1AxisDir.y * t};
                const float s = scoreBezier(m1, q1, m2);
                const Cand  cand{s, t};
                if      (cand.score < best[0].score) { best[2]=best[1]; best[1]=best[0]; best[0]=cand; }
                else if (cand.score < best[1].score) { best[2]=best[1]; best[1]=cand; }
                else if (cand.score < best[2].score) { best[2]=cand; }
            }

            float ts[3] = {best[0].t, best[1].t, best[2].t};
            std::sort(ts, ts + 3);
            tStart = ts[0];
            tEnd   = ts[2];
            if (tEnd - tStart < 1e-4f) break;
        }

        // Final pass in refined range
        for (int i = 0; i < kSamples; ++i)
        {
            const float t  = tStart + (float)i / (float)(kSamples - 1) * (tEnd - tStart);
            const juce::Point<float> q1{M.x + q1AxisDir.x * t,
                                        M.y + q1AxisDir.y * t};
            const float s = scoreBezier(m1, q1, m2);
            if (s < bestCost)
            {
                bestCost = s;
                bestQ0 = m1; bestQ1 = q1; bestQ2 = m2;
                bestM1 = m1; bestM2 = m2;
                found  = true;
            }
        }
    };

    // Family A: partition by Lbf direction; Q1 varies along B (bisDir)
    tryFamily(lineFitDir, bisDir);

    // Family B: partition by bisector B; Q1 varies along Lbf
    tryFamily(bisDir, lineFitDir);

    if (found)
    {
        Q0 = bestQ0; Q1 = bestQ1; Q2 = bestQ2;
        M1 = bestM1; M2 = bestM2;
        parabolaCost = bestCost;
        hasSubMeans  = true;
        hasParabola  = true;
    }
}

float BestFitCanvas::scoreBezier(juce::Point<float> q0, juce::Point<float> q1,
                                   juce::Point<float> q2) const noexcept
{
    // Pre-sample the curve once, then compute min-distance for all input pts.
    juce::Point<float> samples[kBezierSamples];
    for (int i = 0; i < kBezierSamples; ++i)
        samples[i] = evalBezier(q0, q1, q2, (float)i / (float)(kBezierSamples - 1));

    float total = 0.0f;
    for (auto& p : pts)
    {
        float minD = std::numeric_limits<float>::max();
        for (int i = 0; i + 1 < kBezierSamples; ++i)
        {
            const float d = distToSegment(p, samples[i], samples[i + 1]);
            if (d < minD) minD = d;
        }
        total += minD;
    }
    return total;
}

// ─────────────────────────────────────────────────────────────────────────────
// Static geometry helpers
// ─────────────────────────────────────────────────────────────────────────────

float BestFitCanvas::perpDist(juce::Point<float> p,
                               juce::Point<float> linePoint,
                               juce::Point<float> lineDir) noexcept
{
    const float vx = p.x - linePoint.x, vy = p.y - linePoint.y;
    return std::abs(vx * lineDir.y - vy * lineDir.x);
}

float BestFitCanvas::distToSegment(juce::Point<float> p,
                                    juce::Point<float> a,
                                    juce::Point<float> b) noexcept
{
    const float abx = b.x - a.x, aby = b.y - a.y;
    const float len2 = abx * abx + aby * aby;
    if (len2 < 1e-12f)
        return p.getDistanceFrom(a);
    const float t  = std::max(0.0f, std::min(1.0f,
                        ((p.x - a.x) * abx + (p.y - a.y) * aby) / len2));
    const float cx = a.x + t * abx, cy = a.y + t * aby;
    const float dx = p.x - cx,      dy = p.y - cy;
    return std::sqrt(dx * dx + dy * dy);
}

juce::Point<float> BestFitCanvas::evalBezier(juce::Point<float> q0,
                                              juce::Point<float> q1,
                                              juce::Point<float> q2,
                                              float t) noexcept
{
    const float u = 1.0f - t;
    return {u*u*q0.x + 2.0f*u*t*q1.x + t*t*q2.x,
            u*u*q0.y + 2.0f*u*t*q1.y + t*t*q2.y};
}

std::pair<juce::Point<float>, juce::Point<float>>
BestFitCanvas::clipLine(juce::Point<float> lp, juce::Point<float> ld, float w, float h)
{
    float tMin = -1e9f, tMax = 1e9f;

    if (std::abs(ld.x) > 1e-6f)
    {
        float t1 = -lp.x / ld.x, t2 = (w - lp.x) / ld.x;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    }
    if (std::abs(ld.y) > 1e-6f)
    {
        float t1 = -lp.y / ld.y, t2 = (h - lp.y) / ld.y;
        if (t1 > t2) std::swap(t1, t2);
        tMin = std::max(tMin, t1);
        tMax = std::min(tMax, t2);
    }

    return {{lp.x + ld.x * tMin, lp.y + ld.y * tMin},
            {lp.x + ld.x * tMax, lp.y + ld.y * tMax}};
}

// ─────────────────────────────────────────────────────────────────────────────
// Paint
// ─────────────────────────────────────────────────────────────────────────────

void BestFitCanvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1a1a2e));

    const float W = (float)getWidth();
    const float H = (float)getHeight();
    const int   n = (int)pts.size();

    if (n < 2)
    {
        g.setColour(juce::Colours::grey);
        g.setFont(16.0f);
        g.drawText("Left-click to add points  |  Right-click a point to remove it",
                   getLocalBounds(), juce::Justification::centred);
    }

    // Input points
    g.setColour(juce::Colour(0xffffd166));
    for (auto& p : pts)
        g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);

    if (n < 2) return;

    // Best-fit line Lbf
    {
        auto [p1, p2] = clipLine(M, lineFitDir, W, H);
        g.setColour(juce::Colour(0xff06d6a0));
        g.drawLine(p1.x, p1.y, p2.x, p2.y, 2.0f);
    }

    // Bisector B (dashed)
    {
        auto [p1, p2] = clipLine(M, bisDir, W, H);
        juce::Path bisPath;
        bisPath.startNewSubPath(p1);
        bisPath.lineTo(p2);
        juce::Path dashed;
        float dashLengths[] = {8.0f, 5.0f};
        juce::PathStrokeType(1.5f).createDashedStroke(dashed, bisPath, dashLengths, 2);
        g.setColour(juce::Colour(0xff8ecae6));
        g.fillPath(dashed);
    }

    // Mean point M
    g.setColour(juce::Colour(0xffef476f));
    g.fillEllipse(M.x - 6.0f, M.y - 6.0f, 12.0f, 12.0f);
    g.setFont(13.0f);
    g.drawText("M", (int)(M.x + 8), (int)(M.y - 10), 20, 20, juce::Justification::left);

    // Parabola mode extras
    if (showParabola && n >= 3)
    {
        if (hasSubMeans)
        {
            g.setColour(juce::Colour(0xffef476f));
            g.fillEllipse(M1.x - 5.0f, M1.y - 5.0f, 10.0f, 10.0f);
            g.drawText("M1", (int)(M1.x + 6), (int)(M1.y - 10), 26, 20, juce::Justification::left);
            g.fillEllipse(M2.x - 5.0f, M2.y - 5.0f, 10.0f, 10.0f);
            g.drawText("M2", (int)(M2.x + 6), (int)(M2.y - 10), 26, 20, juce::Justification::left);
        }

        if (hasParabola)
        {
            juce::Path bezPath;
            bezPath.startNewSubPath(evalBezier(Q0, Q1, Q2, 0.0f));
            for (int i = 1; i <= 100; ++i)
                bezPath.lineTo(evalBezier(Q0, Q1, Q2, (float)i / 100.0f));
            g.setColour(juce::Colour(0xffff6b6b));
            g.strokePath(bezPath, juce::PathStrokeType(2.5f));
        }
    }

    // Cost overlays
    g.setColour(juce::Colours::white);
    g.setFont(14.0f);
    g.drawText("S(Lbf) = " + juce::String(lineCost, 2),
               8, (int)H - 50, 220, 20, juce::Justification::left);
    if (showParabola && hasParabola)
        g.drawText("S(Cbf) = " + juce::String(parabolaCost, 2),
                   8, (int)H - 28, 220, 20, juce::Justification::left);
}
