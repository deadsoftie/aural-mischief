#pragma once
#include <JuceHeader.h>
#include <map>

// Canvas for Hermite / osculation interpolation (EC-1).
//
// Points are parameterized by t = index (0, 1, 2, ...), matching the Newton
// interpolation canvas convention. Each point may carry:
//   - A first derivative (dy/dt), entered graphically or via dialog.
//     dx/dt is implicitly 1 at every node (unit speed in x).
//   - Higher-order y-derivatives (d^k y / dt^k for k >= 2), entered via dialog.
//     x-derivatives of order >= 2 are implicitly 0.
//
// Conditions are incorporated via the coalescent (confluent) divided-difference
// algorithm: a node with m derivative conditions appears m+1 times in the
// expanded node sequence.

class HermiteInterpCanvas : public juce::Component
{
public:
    HermiteInterpCanvas();

    void clearPoints();
    void clearAllDerivatives();

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;
    void mouseWheelMove(const juce::MouseEvent& e,
                        const juce::MouseWheelDetails& wheel) override;

private:
    // -----------------------------------------------------------------------
    // Data model
    // -----------------------------------------------------------------------
    static constexpr int   maxPoints = 12;
    static constexpr float hitRadius = 10.0f;

    struct HermitePoint
    {
        juce::Point<float>   p;              // screen position
        double               t = 0.0;       // parameter value (set at creation, never changes)
        bool                 hasDeriv1 = false;
        double               slopeY    = 0.0; // dy/dt  (dx/dt implicitly 1 when hasDeriv1)
        std::map<int,double> higherDerivY;    // order k (k>=2) -> d^k y/dt^k
    };

    std::vector<HermitePoint> points;
    int dragIndex = -1;

    // Tangent-drag interaction state
    bool tangentDragActive   = false;
    int  tangentDragPointIdx = -1;

    // -----------------------------------------------------------------------
    // Hermite solver data
    // -----------------------------------------------------------------------
    struct ExpandedNode
    {
        double t;
        int    nodeIdx;  // index into points[]
        int    order;    // repetition order (0 = f value, 1 = f' copy, ...)
    };

    std::vector<ExpandedNode> expandedNodes;
    std::vector<double>       coeffX, coeffY;

    // -----------------------------------------------------------------------
    // Internal helpers
    // -----------------------------------------------------------------------
    int pickPoint(juce::Point<float> pos) const;

    void rebuildHermite();
    void buildExpandedNodes();

    static void buildHermiteCoeffs(
        const std::vector<HermitePoint>& pts,
        const std::vector<ExpandedNode>& nodes,
        std::vector<double>& outCoeffX,
        std::vector<double>& outCoeffY);

    static double evalNewton(
        double t,
        const std::vector<ExpandedNode>& nodes,
        const std::vector<double>& coeffs);

    void buildCurvePolyline(std::vector<juce::Point<float>>& out) const;

    void showContextMenu(int ptIdx);
    void openSlopeDialog(int ptIdx);
    void openHigherDerivDialog(int ptIdx);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HermiteInterpCanvas)
};
