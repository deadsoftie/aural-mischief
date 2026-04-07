#include "HermiteInterpCanvas.h"

HermiteInterpCanvas::HermiteInterpCanvas()
{
    setWantsKeyboardFocus(true);
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

void HermiteInterpCanvas::clearPoints()
{
    points.clear();
    expandedNodes.clear();
    coeffX.clear();
    coeffY.clear();
    dragIndex          = -1;
    tangentDragActive  = false;
    tangentDragPointIdx = -1;
    repaint();
}

void HermiteInterpCanvas::clearAllDerivatives()
{
    for (auto& pt : points)
    {
        pt.hasDeriv1 = false;
        pt.slopeY    = 0.0;
        pt.higherDerivY.clear();
    }
    rebuildHermite();
    repaint();
}

// ---------------------------------------------------------------------------
// Hermite solver
// ---------------------------------------------------------------------------

int HermiteInterpCanvas::pickPoint(juce::Point<float> pos) const
{
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
        if (points[static_cast<size_t>(i)].p.getDistanceSquaredFrom(pos) <= hitRadius * hitRadius)
            return i;
    return -1;
}

void HermiteInterpCanvas::buildExpandedNodes()
{
    expandedNodes.clear();
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        const auto& pt = points[static_cast<size_t>(i)];

        // How many times this node is repeated = highest derivative order + 1
        int maxOrder = 0;
        if (pt.hasDeriv1)
        {
            maxOrder = 1;
            for (const auto& [k, v] : pt.higherDerivY)
                maxOrder = std::max(maxOrder, k);
        }

        for (int r = 0; r <= maxOrder; ++r)
            expandedNodes.push_back({ pt.t, i, r });
    }
}

void HermiteInterpCanvas::buildHermiteCoeffs(
    const std::vector<HermitePoint>& pts,
    const std::vector<ExpandedNode>& nodes,
    std::vector<double>& outCoeffX,
    std::vector<double>& outCoeffY)
{
    const int N = static_cast<int>(nodes.size());
    outCoeffX.assign(static_cast<size_t>(N), 0.0);
    outCoeffY.assign(static_cast<size_t>(N), 0.0);
    if (N == 0) return;

    // In-place divided-difference arrays (start as column 0: function values)
    std::vector<double> ddX(static_cast<size_t>(N)), ddY(static_cast<size_t>(N));
    for (int i = 0; i < N; ++i)
    {
        const auto& pt = pts[static_cast<size_t>(nodes[static_cast<size_t>(i)].nodeIdx)];
        ddX[static_cast<size_t>(i)] = static_cast<double>(pt.p.x);
        ddY[static_cast<size_t>(i)] = static_cast<double>(pt.p.y);
    }
    outCoeffX[0] = ddX[0];
    outCoeffY[0] = ddY[0];

    // Precompute k! for k = 0 .. N
    std::vector<double> fact(static_cast<size_t>(N + 1), 1.0);
    for (int k = 2; k <= N; ++k)
        fact[static_cast<size_t>(k)] = fact[static_cast<size_t>(k - 1)] * static_cast<double>(k);

    // Build columns 1 .. N-1
    for (int k = 1; k < N; ++k)
    {
        for (int i = 0; i < N - k; ++i)
        {
            const double zi  = nodes[static_cast<size_t>(i)].t;
            const double zik = nodes[static_cast<size_t>(i + k)].t;

            if (std::abs(zik - zi) < 1e-12)
            {
                // Coalescent entry: [z_i, ..., z_{i+k}] = f^(k)(z_i) / k!
                // Since all repetitions of a node are consecutive, z_i == z_{i+k}
                // implies all intermediate nodes share the same t value.
                const auto& pt = pts[static_cast<size_t>(nodes[static_cast<size_t>(i)].nodeIdx)];
                double dX_k = 0.0, dY_k = 0.0;

                if (k == 1)
                {
                    if (pt.hasDeriv1)
                    {
                        dX_k = 1.0;       // dx/dt = 1 by convention
                        dY_k = pt.slopeY;
                    }
                }
                else // k >= 2: x-derivative is zero by convention
                {
                    const auto it = pt.higherDerivY.find(k);
                    if (it != pt.higherDerivY.end())
                        dY_k = it->second;
                }

                ddX[static_cast<size_t>(i)] = dX_k / fact[static_cast<size_t>(k)];
                ddY[static_cast<size_t>(i)] = dY_k / fact[static_cast<size_t>(k)];
            }
            else
            {
                // Standard divided difference
                ddX[static_cast<size_t>(i)] =
                    (ddX[static_cast<size_t>(i + 1)] - ddX[static_cast<size_t>(i)]) / (zik - zi);
                ddY[static_cast<size_t>(i)] =
                    (ddY[static_cast<size_t>(i + 1)] - ddY[static_cast<size_t>(i)]) / (zik - zi);
            }
        }
        outCoeffX[static_cast<size_t>(k)] = ddX[0];
        outCoeffY[static_cast<size_t>(k)] = ddY[0];
    }
}

double HermiteInterpCanvas::evalNewton(
    double t,
    const std::vector<ExpandedNode>& nodes,
    const std::vector<double>& coeffs)
{
    const int n = static_cast<int>(coeffs.size());
    if (n == 0) return 0.0;

    // Horner's method for Newton forward form:
    //   p(t) = c[0] + (t-z[0])*(c[1] + (t-z[1])*(c[2] + ...))
    double acc = coeffs[static_cast<size_t>(n - 1)];
    for (int k = n - 2; k >= 0; --k)
        acc = coeffs[static_cast<size_t>(k)] + (t - nodes[static_cast<size_t>(k)].t) * acc;
    return acc;
}

void HermiteInterpCanvas::rebuildHermite()
{
    buildExpandedNodes();
    if (expandedNodes.size() < 2)
    {
        coeffX.clear();
        coeffY.clear();
        return;
    }
    buildHermiteCoeffs(points, expandedNodes, coeffX, coeffY);
}

void HermiteInterpCanvas::buildCurvePolyline(std::vector<juce::Point<float>>& out) const
{
    out.clear();
    if (points.size() < 2 || coeffX.empty()) return;

    const double tMin = points.front().t;
    const double tMax = points.back().t;
    if (tMax <= tMin) return;

    constexpr int samples = 700;
    out.reserve(static_cast<size_t>(samples) + 1);

    for (int s = 0; s <= samples; ++s)
    {
        const double t = tMin + (tMax - tMin) * static_cast<double>(s) / static_cast<double>(samples);
        const float  x = static_cast<float>(evalNewton(t, expandedNodes, coeffX));
        const float  y = static_cast<float>(evalNewton(t, expandedNodes, coeffY));
        out.emplace_back(x, y);
    }
}

// ---------------------------------------------------------------------------
// Mouse events
// ---------------------------------------------------------------------------

void HermiteInterpCanvas::mouseDown(const juce::MouseEvent& e)
{
    if (e.mods.isRightButtonDown())
    {
        // Cancel tangent-drag on right-click before showing menu
        if (tangentDragActive)
        {
            tangentDragActive   = false;
            tangentDragPointIdx = -1;
            rebuildHermite();
            repaint();
        }
        const int idx = pickPoint(e.position);
        if (idx >= 0)
            showContextMenu(idx);
        return;
    }

    // Left click ─ if tangent-drag is active, commit the current slope
    if (tangentDragActive)
    {
        tangentDragActive   = false;
        tangentDragPointIdx = -1;
        rebuildHermite();
        repaint();
        return;
    }

    dragIndex = pickPoint(e.position);
    if (dragIndex < 0)
    {
        // Add new interpolation point
        if (static_cast<int>(points.size()) < maxPoints)
        {
            HermitePoint newPt;
            newPt.p = e.position;
            newPt.t = static_cast<double>(points.size());
            points.push_back(newPt);
            rebuildHermite();
            repaint();
        }
    }
}

void HermiteInterpCanvas::mouseDrag(const juce::MouseEvent& e)
{
    if (dragIndex < 0 || tangentDragActive) return;

    points[static_cast<size_t>(dragIndex)].p = e.position;
    rebuildHermite();
    repaint();
}

void HermiteInterpCanvas::mouseUp(const juce::MouseEvent&)
{
    dragIndex = -1;
}

void HermiteInterpCanvas::mouseMove(const juce::MouseEvent& e)
{
    if (!tangentDragActive) return;
    const int idx = tangentDragPointIdx;
    if (idx < 0 || idx >= static_cast<int>(points.size())) return;

    auto& pt    = points[static_cast<size_t>(idx)];
    const float dx = e.position.x - pt.p.x;
    const float dy = e.position.y - pt.p.y;

    pt.hasDeriv1 = true;
    pt.slopeY    = (std::abs(dx) > 1.0f)
                 ? static_cast<double>(dy / dx)
                 : (dy >= 0.0f ? 1.0e4 : -1.0e4);

    rebuildHermite();
    repaint();
}

void HermiteInterpCanvas::mouseWheelMove(const juce::MouseEvent&,
                                         const juce::MouseWheelDetails& wheel)
{
    if (!tangentDragActive) return;
    const int idx = tangentDragPointIdx;
    if (idx < 0 || idx >= static_cast<int>(points.size())) return;

    auto& pt     = points[static_cast<size_t>(idx)];
    pt.hasDeriv1 = true;
    pt.slopeY   += static_cast<double>(wheel.deltaY) * 0.5;

    rebuildHermite();
    repaint();
}

// ---------------------------------------------------------------------------
// Paint
// ---------------------------------------------------------------------------

void HermiteInterpCanvas::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    // Status / instruction text
    {
        juce::String status;
        if (tangentDragActive && tangentDragPointIdx >= 0)
        {
            status = "Tangent-drag active  \xe2\x80\x94  move mouse to set slope, click to commit, "
                     "scroll wheel for fine adjustment";
        }
        else if (points.size() < 2)
        {
            status = "Left-click to add nodes (max " + juce::String(maxPoints) + ")  |  "
                     "Right-click a node to assign derivatives";
        }
        else
        {
            const int deg = static_cast<int>(expandedNodes.size()) - 1;
            status = "Degree " + juce::String(deg) + " Hermite polynomial  |  "
                   + juce::String(points.size()) + " nodes  "
                   + "(drag to move, right-click to edit derivatives)";
        }
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(15.0f);
        g.drawText(status, getLocalBounds().reduced(10).withHeight(22),
                   juce::Justification::topLeft);
    }

    // Control-point polyline
    if (points.size() >= 2)
    {
        juce::Path poly;
        poly.startNewSubPath(points[0].p);
        for (size_t i = 1; i < points.size(); ++i)
            poly.lineTo(points[i].p);
        g.setColour(juce::Colours::orange.withAlpha(0.45f));
        g.strokePath(poly, juce::PathStrokeType(1.5f));
    }

    // Hermite interpolating curve
    {
        std::vector<juce::Point<float>> curve;
        buildCurvePolyline(curve);
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

    // Tangent segments and control-point dots
    constexpr float tangentLen = 38.0f;

    for (int i = 0; i < static_cast<int>(points.size()); ++i)
    {
        const auto& pt          = points[static_cast<size_t>(i)];
        const bool  isActiveTan = tangentDragActive && (tangentDragPointIdx == i);

        // Tangent segment
        if (pt.hasDeriv1)
        {
            const float angle = std::atan2f(static_cast<float>(pt.slopeY), 1.0f);
            const float tx    = tangentLen * std::cos(angle);
            const float ty    = tangentLen * std::sin(angle);

            g.setColour(isActiveTan ? juce::Colours::yellow
                                    : juce::Colours::limegreen);
            g.drawLine(pt.p.x - tx, pt.p.y - ty,
                       pt.p.x + tx, pt.p.y + ty, 1.8f);

            // Arrowhead in the positive t-direction
            g.fillEllipse(pt.p.x + tx - 3.5f, pt.p.y + ty - 3.5f, 7.0f, 7.0f);

            // Indicate higher derivatives with a small label
            if (!pt.higherDerivY.empty())
            {
                g.setColour(juce::Colours::cyan.withAlpha(0.8f));
                g.setFont(11.0f);
                g.drawText("f''",
                           static_cast<int>(pt.p.x + tx + 5),
                           static_cast<int>(pt.p.y + ty - 7),
                           24, 14, juce::Justification::left);
            }

            // Slope readout while in tangent-drag mode
            if (isActiveTan)
            {
                g.setColour(juce::Colours::yellow.withAlpha(0.9f));
                g.setFont(13.0f);
                g.drawText("m = " + juce::String(pt.slopeY, 3),
                           static_cast<int>(pt.p.x) + 14,
                           static_cast<int>(pt.p.y) + 10,
                           90, 16, juce::Justification::left);
            }
        }

        // Dot
        const bool isDragged = (i == dragIndex);
        juce::Colour dotColour = (isDragged || isActiveTan) ? juce::Colours::yellow
                               : pt.hasDeriv1               ? juce::Colours::limegreen
                                                             : juce::Colours::cyan;
        g.setColour(dotColour);
        g.fillEllipse(pt.p.x - 6.0f, pt.p.y - 6.0f, 12.0f, 12.0f);
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.drawEllipse(pt.p.x - 6.0f, pt.p.y - 6.0f, 12.0f, 12.0f, 1.0f);

        // Label
        g.setColour(juce::Colours::white.withAlpha(0.85f));
        g.setFont(13.0f);
        g.drawText("P" + juce::String(i),
                   static_cast<int>(pt.p.x) + 9,
                   static_cast<int>(pt.p.y) - 9,
                   30, 16, juce::Justification::left);
    }
}

// ---------------------------------------------------------------------------
// Dialogs and context menu
// ---------------------------------------------------------------------------

void HermiteInterpCanvas::showContextMenu(int ptIdx)
{
    const auto& pt = points[static_cast<size_t>(ptIdx)];
    const bool  hasAnyDeriv = pt.hasDeriv1 || !pt.higherDerivY.empty();

    juce::PopupMenu menu;
    menu.addSectionHeader("P" + juce::String(ptIdx) + " \xe2\x80\x94 derivatives");
    menu.addItem(1, "Set slope graphically");
    menu.addItem(2, "Set slope by value...");
    menu.addSeparator();
    menu.addItem(3, "Set higher derivatives...", pt.hasDeriv1);
    menu.addSeparator();
    menu.addItem(4, "Clear derivatives", hasAnyDeriv);

    // Use a SafePointer so the lambda is safe if the canvas is torn down
    juce::Component::SafePointer<HermiteInterpCanvas> safe(this);
    menu.showMenuAsync(juce::PopupMenu::Options{}.withTargetComponent(this),
        [safe, ptIdx](int result)
        {
            if (safe == nullptr || result == 0) return;
            auto& canvas = *safe;

            if (result == 1)
            {
                canvas.tangentDragActive   = true;
                canvas.tangentDragPointIdx = ptIdx;
                canvas.points[static_cast<size_t>(ptIdx)].hasDeriv1 = true;
                canvas.rebuildHermite();
                canvas.repaint();
            }
            else if (result == 2)
            {
                canvas.openSlopeDialog(ptIdx);
            }
            else if (result == 3)
            {
                canvas.openHigherDerivDialog(ptIdx);
            }
            else if (result == 4)
            {
                auto& pt = canvas.points[static_cast<size_t>(ptIdx)];
                pt.hasDeriv1 = false;
                pt.slopeY    = 0.0;
                pt.higherDerivY.clear();
                canvas.rebuildHermite();
                canvas.repaint();
            }
        });
}

void HermiteInterpCanvas::openSlopeDialog(int ptIdx)
{
    const auto& pt = points[static_cast<size_t>(ptIdx)];

    auto* w = new juce::AlertWindow("Set Slope - P" + juce::String(ptIdx),
                                    "dy/dt at this node (dx/dt is implicitly 1):",
                                    juce::MessageBoxIconType::QuestionIcon);
    w->addTextEditor("slope",
                     juce::String(pt.hasDeriv1 ? pt.slopeY : 0.0, 4),
                     "dy/dt:");
    w->addButton("OK", 1);
    w->addButton("Cancel", 0);

    juce::Component::SafePointer<HermiteInterpCanvas> safe(this);
    w->enterModalState(true,
        juce::ModalCallbackFunction::create([safe, ptIdx, w](int result)
        {
            if (safe != nullptr && result == 1)
            {
                auto& p = safe->points[static_cast<size_t>(ptIdx)];
                p.hasDeriv1 = true;
                p.slopeY    = w->getTextEditorContents("slope").getDoubleValue();
                safe->rebuildHermite();
                safe->repaint();
            }
            delete w;
        }),
        false);
}

void HermiteInterpCanvas::openHigherDerivDialog(int ptIdx)
{
    const auto& pt = points[static_cast<size_t>(ptIdx)];
    if (!pt.hasDeriv1) return; // first derivative must be set first

    const auto getD = [&](int k) -> double {
        const auto it = pt.higherDerivY.find(k);
        return it != pt.higherDerivY.end() ? it->second : 0.0;
    };

    auto* w = new juce::AlertWindow("Higher Derivatives - P" + juce::String(ptIdx),
                                    "Enter y-derivatives (x-derivatives assumed zero for k >= 2):",
                                    juce::MessageBoxIconType::QuestionIcon);
    w->addTextEditor("d2", juce::String(getD(2), 4), "d2y/dt2:");
    w->addTextEditor("d3", juce::String(getD(3), 4), "d3y/dt3:");
    w->addButton("OK", 1);
    w->addButton("Cancel", 0);

    juce::Component::SafePointer<HermiteInterpCanvas> safe(this);
    w->enterModalState(true,
        juce::ModalCallbackFunction::create([safe, ptIdx, w](int result)
        {
            if (safe != nullptr && result == 1)
            {
                auto& p      = safe->points[static_cast<size_t>(ptIdx)];
                const double d2 = w->getTextEditorContents("d2").getDoubleValue();
                const double d3 = w->getTextEditorContents("d3").getDoubleValue();

                if (d2 != 0.0) p.higherDerivY[2] = d2; else p.higherDerivY.erase(2);
                if (d3 != 0.0) p.higherDerivY[3] = d3; else p.higherDerivY.erase(3);

                safe->rebuildHermite();
                safe->repaint();
            }
            delete w;
        }),
        false);
}
