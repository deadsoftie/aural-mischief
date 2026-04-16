#include "BezierCanvas3D.h"
#include <cmath>

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------
BezierCanvas3D::BezierCanvas3D()
{
    buildChooseTable(20);
    setWantsKeyboardFocus(true);
}

// ---------------------------------------------------------------------------
// Public setters
// ---------------------------------------------------------------------------
void BezierCanvas3D::setDegree(int d)
{
    degree = juce::jlimit(1, 20, d);
    const int maxPts = degree + 1;
    if (static_cast<int>(controlPoints.size()) > maxPts)
        controlPoints.resize(static_cast<size_t>(maxPts));
    repaint();
}

void BezierCanvas3D::setMethod(Method m)
{
    method = m;
    repaint();
}

void BezierCanvas3D::setT(double tt)
{
    t = juce::jlimit(0.0, 1.0, tt);
    repaint();
}

void BezierCanvas3D::clearPoints()
{
    controlPoints.clear();
    dragIndex = -1;
    repaint();
}

void BezierCanvas3D::resetCamera()
{
    camAzimuth   =  0.4f;
    camElevation =  0.35f;
    camDistance  = 10.0f;
    camPivot     = { 0.0f, 0.0f, 0.0f };
    repaint();
}

void BezierCanvas3D::resetToDefaultIfEmpty()
{
    if (!controlPoints.empty()) return;
    controlPoints = {
        { -3.0f,  0.0f,  0.0f },
        { -1.0f,  2.0f, -2.0f },
        {  1.0f,  2.0f,  2.0f },
        {  3.0f,  0.0f,  0.0f }
    };
}

// ---------------------------------------------------------------------------
// Pascal triangle
// ---------------------------------------------------------------------------
void BezierCanvas3D::buildChooseTable(int maxDegree)
{
    const size_t sz = static_cast<size_t>(maxDegree) + 1;
    choose.assign(sz, std::vector<double>(sz, 0.0));
    for (int n = 0; n <= maxDegree; ++n)
    {
        choose[static_cast<size_t>(n)][0] = 1.0;
        choose[static_cast<size_t>(n)][static_cast<size_t>(n)] = 1.0;
        for (int k = 1; k < n; ++k)
            choose[static_cast<size_t>(n)][static_cast<size_t>(k)] =
                choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k - 1)] +
                choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k)];
    }
}

// ---------------------------------------------------------------------------
// Camera helpers
// ---------------------------------------------------------------------------
Point3D BezierCanvas3D::getEye() const noexcept
{
    return {
        camPivot.x + camDistance * std::cos(camElevation) * std::sin(camAzimuth),
        camPivot.y + camDistance * std::sin(camElevation),
        camPivot.z + camDistance * std::cos(camElevation) * std::cos(camAzimuth)
    };
}

void BezierCanvas3D::getCameraAxes(Point3D& right, Point3D& up, Point3D& fwd) const noexcept
{
    const Point3D eye  = getEye();
    fwd = normalize3({ camPivot.x - eye.x, camPivot.y - eye.y, camPivot.z - eye.z });

    const Point3D worldUp = { 0.0f, 1.0f, 0.0f };
    Point3D candidate = cross3(fwd, worldUp);

    // Guard against gimbal lock (elevation near ±90°)
    if (len3(candidate) < 0.001f)
        candidate = { 1.0f, 0.0f, 0.0f };

    right = normalize3(candidate);
    up    = cross3(right, fwd);   // already unit: right ⊥ fwd, both unit
}

bool BezierCanvas3D::projectPoint(Point3D p,
                                   Point3D eyePos, Point3D right, Point3D up, Point3D fwd,
                                   float focalLen, float cx, float screenCy,
                                   juce::Point<float>& out) const noexcept
{
    const Point3D rel = p - eyePos;
    const float pcz = dot3(rel, fwd);
    if (pcz < 0.01f) return false;

    const float pcx = dot3(rel, right);
    const float pcy = dot3(rel, up);

    out = { cx + pcx / pcz * focalLen,
            screenCy - pcy / pcz * focalLen };  // flip Y: screen Y increases downward
    return true;
}

bool BezierCanvas3D::projectPointFull(Point3D p, juce::Point<float>& out) const noexcept
{
    if (getWidth() < 1) return false;
    Point3D right, up, fwd;
    getCameraAxes(right, up, fwd);
    const float focalLen = static_cast<float>(getWidth()) * 0.65f;
    return projectPoint(p, getEye(), right, up, fwd,
                        focalLen,
                        static_cast<float>(getWidth())  * 0.5f,
                        static_cast<float>(getHeight()) * 0.5f,
                        out);
}

// ---------------------------------------------------------------------------
// Hit testing
// ---------------------------------------------------------------------------
int BezierCanvas3D::pickPoint(juce::Point<float> screenPos) const noexcept
{
    constexpr float r2 = hitRadius * hitRadius;
    for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i)
    {
        juce::Point<float> sp;
        if (projectPointFull(controlPoints[static_cast<size_t>(i)], sp) &&
            sp.getDistanceSquaredFrom(screenPos) <= r2)
            return i;
    }
    return -1;
}

// ---------------------------------------------------------------------------
// Unproject screen click → world Y = 0 plane
// ---------------------------------------------------------------------------
Point3D BezierCanvas3D::unprojectToGroundPlane(juce::Point<float> screenPos) const noexcept
{
    Point3D right, up, fwd;
    getCameraAxes(right, up, fwd);
    const Point3D eye      = getEye();
    const float   focalLen = static_cast<float>(getWidth()) * 0.65f;
    const float   ndcX     = screenPos.x - static_cast<float>(getWidth())  * 0.5f;
    const float   ndcY     = screenPos.y - static_cast<float>(getHeight()) * 0.5f;

    // Direction of ray through this screen pixel
    Point3D rayDir = {
        fwd.x * focalLen + right.x * ndcX - up.x * ndcY,
        fwd.y * focalLen + right.y * ndcX - up.y * ndcY,
        fwd.z * focalLen + right.z * ndcX - up.z * ndcY
    };
    rayDir = normalize3(rayDir);

    // Intersect ray with Y = 0 plane: eye.y + tParam * rayDir.y = 0
    if (std::abs(rayDir.y) < 1e-5f)
        return camPivot;                // ray parallel to ground — place at pivot

    const float tParam = -eye.y / rayDir.y;
    if (tParam <= 0.0f)
        return camPivot;                // intersection behind camera

    return { eye.x + tParam * rayDir.x,
             0.0f,
             eye.z + tParam * rayDir.z };
}

// ---------------------------------------------------------------------------
// Bézier evaluation — 3D
// ---------------------------------------------------------------------------
Point3D BezierCanvas3D::evalNLI3D(double tt,
                                    std::vector<std::vector<Point3D>>* outTri) const
{
    const int n = static_cast<int>(controlPoints.size());
    if (n == 0) return {};
    if (n == 1) return controlPoints[0];

    const int dEff = n - 1;
    std::vector<std::vector<Point3D>> tri(static_cast<size_t>(dEff) + 1);
    tri[0] = controlPoints;

    for (int r = 1; r <= dEff; ++r)
    {
        tri[static_cast<size_t>(r)].resize(static_cast<size_t>(dEff - r + 1));
        for (int i = 0; i <= dEff - r; ++i)
            tri[static_cast<size_t>(r)][static_cast<size_t>(i)] =
                lerp3(tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i)],
                      tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i) + 1],
                      tt);
    }

    if (outTri) *outTri = tri;
    return tri[static_cast<size_t>(dEff)][0];
}

Point3D BezierCanvas3D::evalBB3D(double tt) const
{
    const int n = static_cast<int>(controlPoints.size());
    if (n == 0) return {};
    if (n == 1) return controlPoints[0];

    const int    dEff = n - 1;
    const double u    = 1.0 - tt;
    double sumX = 0.0, sumY = 0.0, sumZ = 0.0;

    for (int i = 0; i <= dEff; ++i)
    {
        const double basis = choose[static_cast<size_t>(dEff)][static_cast<size_t>(i)] *
                             std::pow(u, dEff - i) * std::pow(tt, i);
        const auto& p = controlPoints[static_cast<size_t>(i)];
        sumX += static_cast<double>(p.x) * basis;
        sumY += static_cast<double>(p.y) * basis;
        sumZ += static_cast<double>(p.z) * basis;
    }

    return { static_cast<float>(sumX),
             static_cast<float>(sumY),
             static_cast<float>(sumZ) };
}

void BezierCanvas3D::subdivideMidpoint3D(const std::vector<Point3D>& pts, int depth,
                                          std::vector<Point3D>& out) const
{
    if (depth <= 0)
    {
        if (out.empty()) out.push_back(pts.front());
        for (size_t i = 1; i < pts.size(); ++i)
            out.push_back(pts[i]);
        return;
    }

    const int dLocal = static_cast<int>(pts.size()) - 1;
    std::vector<std::vector<Point3D>> tri(pts.size());
    tri[0] = pts;

    for (int r = 1; r <= dLocal; ++r)
    {
        tri[static_cast<size_t>(r)].resize(static_cast<size_t>(dLocal - r + 1));
        for (int i = 0; i <= dLocal - r; ++i)
            tri[static_cast<size_t>(r)][static_cast<size_t>(i)] =
                lerp3(tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i)],
                      tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i) + 1],
                      0.5);
    }

    std::vector<Point3D> left, right;
    left.reserve(pts.size());
    right.reserve(pts.size());

    for (int r = 0; r <= dLocal; ++r)
        left.push_back(tri[static_cast<size_t>(r)][0]);
    for (int r = dLocal; r >= 0; --r)
        right.push_back(tri[static_cast<size_t>(r)][static_cast<size_t>(dLocal - r)]);

    subdivideMidpoint3D(left,  depth - 1, out);
    if (!out.empty()) out.pop_back();
    subdivideMidpoint3D(right, depth - 1, out);
}

void BezierCanvas3D::buildPolyLine3D(std::vector<Point3D>& out) const
{
    out.clear();
    if (controlPoints.size() < 2) return;

    if (method == Method::Midpoint)
    {
        subdivideMidpoint3D(controlPoints, 6, out);
        return;
    }

    constexpr int samples = 600;
    out.reserve(samples + 1);
    for (int s = 0; s <= samples; ++s)
    {
        const double tt = static_cast<double>(s) / static_cast<double>(samples);
        out.push_back(method == Method::NLI ? evalNLI3D(tt) : evalBB3D(tt));
    }
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------
void BezierCanvas3D::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    if (getWidth() < 1) return;

    // Pre-compute camera state once for this paint call
    Point3D right, up, fwd;
    getCameraAxes(right, up, fwd);
    const Point3D eye      = getEye();
    const float   focalLen = static_cast<float>(getWidth())  * 0.65f;
    const float   scrCx    = static_cast<float>(getWidth())  * 0.5f;
    const float   scrCy    = static_cast<float>(getHeight()) * 0.5f;

    // Project helper (captures camera state by ref)
    auto proj = [&](Point3D p, juce::Point<float>& out) -> bool
    {
        return projectPoint(p, eye, right, up, fwd, focalLen, scrCx, scrCy, out);
    };

    // --- XZ grid at Y = 0 ---
    {
        g.setColour(juce::Colours::white.withAlpha(0.07f));
        constexpr int ext = 5;
        for (int i = -ext; i <= ext; ++i)
        {
            juce::Point<float> a, b;
            if (proj({ static_cast<float>(i), 0.0f, -static_cast<float>(ext) }, a) &&
                proj({ static_cast<float>(i), 0.0f,  static_cast<float>(ext) }, b))
                g.drawLine(a.x, a.y, b.x, b.y, 1.0f);

            if (proj({ -static_cast<float>(ext), 0.0f, static_cast<float>(i) }, a) &&
                proj({  static_cast<float>(ext), 0.0f, static_cast<float>(i) }, b))
                g.drawLine(a.x, a.y, b.x, b.y, 1.0f);
        }
    }

    // --- World axes ---
    {
        struct Axis { Point3D from; Point3D to; juce::Colour col; const char* label; };
        const Axis axes[] = {
            { {0,0,0}, {3,0,0}, juce::Colours::red,   "X" },
            { {0,0,0}, {0,3,0}, juce::Colours::green, "Y" },
            { {0,0,0}, {0,0,3}, juce::Colours::blue,  "Z" },
        };
        for (const auto& ax : axes)
        {
            juce::Point<float> pA, pB;
            if (!proj(ax.from, pA) || !proj(ax.to, pB)) continue;
            g.setColour(ax.col);
            g.drawLine(pA.x, pA.y, pB.x, pB.y, 2.5f);
            g.setFont(13.0f);
            g.drawText(ax.label, static_cast<int>(pB.x) + 4, static_cast<int>(pB.y) - 8,
                       20, 16, juce::Justification::left);
        }
    }

    // --- Control polygon ---
    if (controlPoints.size() >= 2)
    {
        g.setColour(juce::Colours::orange.withAlpha(0.9f));
        for (size_t i = 1; i < controlPoints.size(); ++i)
        {
            juce::Point<float> pA, pB;
            if (proj(controlPoints[i - 1], pA) && proj(controlPoints[i], pB))
                g.drawLine(pA.x, pA.y, pB.x, pB.y, 2.0f);
        }
    }

    // --- Bézier curve ---
    if (controlPoints.size() >= 2)
    {
        std::vector<Point3D> curve;
        buildPolyLine3D(curve);

        if (curve.size() >= 2)
        {
            juce::Path path;
            bool started = false;
            for (const auto& p3 : curve)
            {
                juce::Point<float> sp;
                if (!proj(p3, sp)) { started = false; continue; }
                if (!started) { path.startNewSubPath(sp); started = true; }
                else          { path.lineTo(sp); }
            }
            g.setColour(juce::Colours::white);
            g.strokePath(path, juce::PathStrokeType(2.5f));
        }

        // --- NLI shells (De Casteljau triangle) ---
        if (method == Method::NLI)
        {
            std::vector<std::vector<Point3D>> tri;
            const Point3D pt3 = evalNLI3D(t, &tri);
            const int dEff = static_cast<int>(controlPoints.size()) - 1;

            g.setColour(juce::Colours::cyan.withAlpha(0.25f));
            for (int r = 0; r < dEff; ++r)
                for (int i = 0; i < dEff - r; ++i)
                {
                    juce::Point<float> pA, pB;
                    if (proj(tri[static_cast<size_t>(r)][static_cast<size_t>(i)],     pA) &&
                        proj(tri[static_cast<size_t>(r)][static_cast<size_t>(i) + 1], pB))
                        g.drawLine(pA.x, pA.y, pB.x, pB.y, 1.5f);
                }

            g.setColour(juce::Colours::cyan.withAlpha(0.6f));
            for (int r = 0; r <= dEff; ++r)
                for (int i = 0; i <= dEff - r; ++i)
                {
                    juce::Point<float> sp;
                    if (proj(tri[static_cast<size_t>(r)][static_cast<size_t>(i)], sp))
                        g.fillEllipse(sp.x - 3.0f, sp.y - 3.0f, 6.0f, 6.0f);
                }

            juce::Point<float> ptSp;
            if (proj(pt3, ptSp))
            {
                g.setColour(juce::Colours::yellow);
                g.fillEllipse(ptSp.x - 5.0f, ptSp.y - 5.0f, 10.0f, 10.0f);
            }
        }
        else
        {
            // Moving point for BB / Midpoint
            const Point3D pt3 = (method == Method::BB) ? evalBB3D(t) : evalNLI3D(t);
            juce::Point<float> ptSp;
            if (proj(pt3, ptSp))
            {
                g.setColour(juce::Colours::yellow);
                g.fillEllipse(ptSp.x - 5.0f, ptSp.y - 5.0f, 10.0f, 10.0f);
            }
        }
    }

    // --- Control points ---
    g.setFont(13.0f);
    for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i)
    {
        juce::Point<float> sp;
        if (!proj(controlPoints[static_cast<size_t>(i)], sp)) continue;

        g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
        g.fillEllipse(sp.x - 6.0f, sp.y - 6.0f, 12.0f, 12.0f);

        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.drawText("P" + juce::String(i),
                   static_cast<int>(sp.x) + 8, static_cast<int>(sp.y) - 8,
                   40, 16, juce::Justification::left);
    }

    // --- Instruction text ---
    if (static_cast<int>(controlPoints.size()) < degree + 1)
    {
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(16.0f);
        g.drawText("Click to add control points (" +
                   juce::String(controlPoints.size()) + "/" + juce::String(degree + 1) + ")",
                   getLocalBounds().reduced(10), juce::Justification::topLeft);
    }

    // --- HUD ---
    {
        auto bounds = getLocalBounds().reduced(10);
        g.setColour(juce::Colours::white.withAlpha(0.5f));
        g.setFont(11.0f);

        const juce::String controls =
            "Left-click: add pt  |  Left-drag pt: move XZ  |  Shift+drag pt: move Y  "
            "|  Right-drag: orbit  |  Scroll: zoom";
        g.drawText(controls, bounds, juce::Justification::bottomRight);

        const juce::String camInfo =
            "Az: " + juce::String(juce::radiansToDegrees(camAzimuth),   1) + u8"\u00B0" +
            "  Elev: " + juce::String(juce::radiansToDegrees(camElevation), 1) + u8"\u00B0" +
            "  Dist: " + juce::String(camDistance, 1);
        g.drawText(camInfo, bounds.removeFromBottom(32).removeFromBottom(16),
                   juce::Justification::bottomLeft);
    }
}

// ---------------------------------------------------------------------------
// Mouse interaction
// ---------------------------------------------------------------------------
void BezierCanvas3D::mouseDown(const juce::MouseEvent& e)
{
    lastMousePos = e.position;

    if (e.mods.isRightButtonDown())
    {
        orbitDragging = true;
        return;
    }

    dragIndex = pickPoint(e.position);

    if (dragIndex < 0)
    {
        const int maxPts = degree + 1;
        if (static_cast<int>(controlPoints.size()) < maxPts)
        {
            controlPoints.push_back(unprojectToGroundPlane(e.position));
            repaint();
        }
    }
}

void BezierCanvas3D::mouseDrag(const juce::MouseEvent& e)
{
    const float dx = e.position.x - lastMousePos.x;
    const float dy = e.position.y - lastMousePos.y;
    lastMousePos = e.position;

    if (orbitDragging)
    {
        camAzimuth   += dx * 0.005f;
        camElevation -= dy * 0.005f;
        camElevation  = juce::jlimit(-1.4f, 1.4f, camElevation);
        repaint();
        return;
    }

    if (dragIndex >= 0)
    {
        Point3D axRight, axUp, axFwd;
        getCameraAxes(axRight, axUp, axFwd);

        auto& pt = controlPoints[static_cast<size_t>(dragIndex)];

        const Point3D rel = pt - getEye();
        const float   dist     = len3(rel);
        const float   focalLen = static_cast<float>(getWidth()) * 0.65f;
        const float   scale    = (focalLen > 0.0f) ? dist / focalLen : 0.01f;

        if (e.mods.isShiftDown())
        {
            // Shift+drag → move along world Y only
            pt.y -= dy * scale;
        }
        else
        {
            // No modifier → move in screen-aligned (camera right / up) plane
            pt.x += dx * scale * axRight.x - dy * scale * axUp.x;
            pt.y += dx * scale * axRight.y - dy * scale * axUp.y;
            pt.z += dx * scale * axRight.z - dy * scale * axUp.z;
        }

        repaint();
    }
}

void BezierCanvas3D::mouseUp(const juce::MouseEvent&)
{
    dragIndex     = -1;
    orbitDragging = false;
}

void BezierCanvas3D::mouseWheelMove(const juce::MouseEvent&,
                                     const juce::MouseWheelDetails& d)
{
    camDistance *= 1.0f - d.deltaY * 0.15f;
    camDistance  = juce::jlimit(1.0f, 100.0f, camDistance);
    repaint();
}
