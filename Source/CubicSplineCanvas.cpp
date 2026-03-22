#include "CubicSplineCanvas.h"

CubicSplineCanvas::CubicSplineCanvas() {}

void CubicSplineCanvas::setSegments(int k)
{
    maxSegments = juce::jlimit(1, 20, k);

    const int maxPts = maxSegments + 1;
    if (static_cast<int>(points.size()) > maxPts)
        points.resize(static_cast<size_t>(maxPts));

    rebuildSpline();
    repaint();
}

void CubicSplineCanvas::clearPoints()
{
    points.clear();
    dragIndex = -1;
    coeffX.clear();
    coeffY.clear();
    repaint();
}

int CubicSplineCanvas::pickPoint(juce::Point<float> mousePos) const
{
    const float r2 = hitRadius * hitRadius;
    for (int i = 0; i < static_cast<int>(points.size()); ++i)
        if (points[static_cast<size_t>(i)].getDistanceSquaredFrom(mousePos) <= r2)
            return i;
    return -1;
}

bool CubicSplineCanvas::gaussElim(std::vector<std::vector<double>> &A,
                                  std::vector<double> &rhs,
                                  std::vector<double> &sol)
{
    const int n = static_cast<int>(rhs.size());

    for (int col = 0; col < n; ++col)
    {
        // Partial pivot
        int pivot = col;
        for (int row = col + 1; row < n; ++row)
            if (std::abs(A[static_cast<size_t>(row)][static_cast<size_t>(col)]) >
                std::abs(A[static_cast<size_t>(pivot)][static_cast<size_t>(col)]))
                pivot = row;

        if (std::abs(A[static_cast<size_t>(pivot)][static_cast<size_t>(col)]) < 1e-12)
            return false;

        std::swap(A[static_cast<size_t>(col)], A[static_cast<size_t>(pivot)]);
        std::swap(rhs[static_cast<size_t>(col)], rhs[static_cast<size_t>(pivot)]);

        for (int row = col + 1; row < n; ++row)
        {
            const double factor = A[static_cast<size_t>(row)][static_cast<size_t>(col)] / A[static_cast<size_t>(col)][static_cast<size_t>(col)];
            for (int j = col; j < n; ++j)
                A[static_cast<size_t>(row)][static_cast<size_t>(j)] -=
                    factor * A[static_cast<size_t>(col)][static_cast<size_t>(j)];
            rhs[static_cast<size_t>(row)] -= factor * rhs[static_cast<size_t>(col)];
        }
    }

    sol.resize(static_cast<size_t>(n));
    for (int i = n - 1; i >= 0; --i)
    {
        sol[static_cast<size_t>(i)] = rhs[static_cast<size_t>(i)];
        for (int j = i + 1; j < n; ++j)
            sol[static_cast<size_t>(i)] -= A[static_cast<size_t>(i)][static_cast<size_t>(j)] * sol[static_cast<size_t>(j)];
        sol[static_cast<size_t>(i)] /= A[static_cast<size_t>(i)][static_cast<size_t>(i)];
    }
    return true;
}

double CubicSplineCanvas::evalSpline(double t, const std::vector<double> &c, int k)
{
    // f(t) = c[0] + c[1]*t + c[2]*t^2 + c[3]*t^3 + sum_{j=1}^{k-1} c[3+j]*(t-j)^3_+
    double val = c[0] + c[1] * t + c[2] * t * t + c[3] * t * t * t;
    for (int j = 1; j <= k - 1; ++j)
    {
        const double s = t - static_cast<double>(j);
        if (s > 0.0)
            val += c[static_cast<size_t>(3 + j)] * s * s * s;
    }
    return val;
}

void CubicSplineCanvas::rebuildSpline()
{
    coeffX.clear();
    coeffY.clear();

    const int n = static_cast<int>(points.size());
    if (n < 2)
        return;

    const int k = n - 1;  // number of sub-intervals, t in [0, k]
    const int sz = n + 2; // system size

    std::vector<std::vector<double>> A(static_cast<size_t>(sz),
                                       std::vector<double>(static_cast<size_t>(sz), 0.0));

    for (int i = 0; i < n; ++i)
    {
        const double ti = static_cast<double>(i);
        A[static_cast<size_t>(i)][0] = 1.0;
        A[static_cast<size_t>(i)][1] = ti;
        A[static_cast<size_t>(i)][2] = ti * ti;
        A[static_cast<size_t>(i)][3] = ti * ti * ti;
        for (int j = 1; j <= k - 1; ++j)
        {
            const double s = ti - static_cast<double>(j);
            A[static_cast<size_t>(i)][static_cast<size_t>(3 + j)] = (s > 0.0) ? s * s * s : 0.0;
        }
    }

    // f''(0) = 0: only the t^2 term contributes at t=0
    A[static_cast<size_t>(n)][2] = 2.0;

    // f''(k) = 0
    const double tk = static_cast<double>(k);
    A[static_cast<size_t>(n + 1)][2] = 2.0;
    A[static_cast<size_t>(n + 1)][3] = 6.0 * tk;
    for (int j = 1; j <= k - 1; ++j)
        A[static_cast<size_t>(n + 1)][static_cast<size_t>(3 + j)] = 6.0 * (tk - static_cast<double>(j));

    std::vector<double> rhsX(static_cast<size_t>(sz), 0.0);
    std::vector<double> rhsY(static_cast<size_t>(sz), 0.0);
    for (int i = 0; i < n; ++i)
    {
        rhsX[static_cast<size_t>(i)] = static_cast<double>(points[static_cast<size_t>(i)].x);
        rhsY[static_cast<size_t>(i)] = static_cast<double>(points[static_cast<size_t>(i)].y);
    }

    auto Ax = A;
    auto Ay = A;
    gaussElim(Ax, rhsX, coeffX);
    gaussElim(Ay, rhsY, coeffY);
}

void CubicSplineCanvas::buildCurvePolyline(std::vector<juce::Point<float>> &out) const
{
    out.clear();
    const int n = static_cast<int>(points.size());
    if (n < 2 || coeffX.empty() || coeffY.empty())
        return;

    const int k = n - 1;
    constexpr int samples = 700;
    out.reserve(static_cast<size_t>(samples + 1));

    for (int s = 0; s <= samples; ++s)
    {
        const double t = static_cast<double>(k) * static_cast<double>(s) / static_cast<double>(samples);
        const float x = static_cast<float>(evalSpline(t, coeffX, k));
        const float y = static_cast<float>(evalSpline(t, coeffY, k));
        out.emplace_back(x, y);
    }
}

void CubicSplineCanvas::mouseDown(const juce::MouseEvent &e)
{
    dragIndex = pickPoint(e.position);
    if (dragIndex < 0)
    {
        if (static_cast<int>(points.size()) < maxSegments + 1)
        {
            points.push_back(e.position);
            rebuildSpline();
            repaint();
        }
    }
}

void CubicSplineCanvas::mouseDrag(const juce::MouseEvent &e)
{
    if (dragIndex < 0)
        return;
    points[static_cast<size_t>(dragIndex)] = e.position;
    rebuildSpline();
    repaint();
}

void CubicSplineCanvas::mouseUp(const juce::MouseEvent &)
{
    dragIndex = -1;
    repaint();
}

void CubicSplineCanvas::paint(juce::Graphics &g)
{
    g.fillAll(juce::Colours::darkgrey.darker(0.7f));

    const int n = static_cast<int>(points.size());

    if (n < maxSegments + 1)
    {
        g.setColour(juce::Colours::white.withAlpha(0.7f));
        g.setFont(16.0f);
        g.drawText("Click to add interpolation points (" +
                       juce::String(n) + "/" + juce::String(maxSegments + 1) + ")",
                   getLocalBounds().reduced(10), juce::Justification::topLeft);
    }

    // Polyline connecting the interpolation points
    if (n >= 2)
    {
        juce::Path poly;
        poly.startNewSubPath(points[0]);
        for (int i = 1; i < n; ++i)
            poly.lineTo(points[static_cast<size_t>(i)]);
        g.setColour(juce::Colours::orange.withAlpha(0.5f));
        g.strokePath(poly, juce::PathStrokeType(1.5f));
    }

    // Natural cubic spline curve
    if (n >= 2)
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

    // Interpolation points + labels
    for (int i = 0; i < n; ++i)
    {
        const auto &pt = points[static_cast<size_t>(i)];
        g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
        g.fillEllipse(pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.drawText("P" + juce::String(i),
                   static_cast<int>(pt.x) + 8, static_cast<int>(pt.y) - 8, 40, 16,
                   juce::Justification::left);
    }
}
