#pragma once
#include <JuceHeader.h>

// Interactive canvas for Part VI: De Boor Algorithm for Polynomial Curves.
//
// Bezier mode (default):
//   - User clicks to place control points P[0..d] incrementally.
//   - Renders the Bezier curve via NLI (De Casteljau) and the control polyline.
//   - Knot sequence displayed is the clamped form: [0,...,0, 1,...,1].
//
// De Boor mode:
//   - Spread knot sequence is built and De Boor control points Q[0..d]
//     are computed from the polar form F[u1,...,ud] of the Bezier curve.
//   - Renders the identical curve via the De Boor NLI recurrence.
//   - Q points are independently draggable; Bezier polygon shown dimmed.
//   - Switching back to Bezier reconstructs P from the current Q via the
//     inverse blossom, so the curve is continuous across mode switches.

class DeBoorPolyCanvas : public juce::Component
{
public:
    DeBoorPolyCanvas();

    void setDegree(int newDegree);
    void clearPoints();
    void switchKnots();          // toggle clamped <-> spread
    bool isDeBoorMode() const    { return deBoorModeActive; }
    juce::String getKnotString() const;

    // Called whenever the knot sequence changes (point added, degree changed, mode switched)
    std::function<void()> onKnotsChanged;

    void paint    (juce::Graphics& g)          override;
    void mouseDown(const juce::MouseEvent& e)  override;
    void mouseDrag(const juce::MouseEvent& e)  override;
    void mouseUp  (const juce::MouseEvent& e)  override;

private:
    int  degree           = 3;
    bool deBoorModeActive = false;

    std::vector<juce::Point<float>> bezierPts;   // Bezier control points P[0..d]
    std::vector<juce::Point<float>> deBoorPts;   // DeBoor control points Q[0..d]
    std::vector<double>             knots;        // active knot sequence

    int dragIndex = -1;

    // Knot management
    void buildClampedKnots();
    void buildSpreadKnots(int d);   // d = effective degree
    void computeDeBoorPointsFromPolarForm();
    void computeBezierFromDeBoor();  // inverse: reconstruct P from Q via blossom

    // Polar form: F[args[0],...,args[d-1]] computed via NLI on bezierPts
    juce::Point<float> polarForm(const std::vector<double>& args) const;

    // Bezier NLI (De Casteljau)
    juce::Point<float> evalNLI(double tt) const;

    // De Boor NLI recurrence over deBoorPts + spread knots
    juce::Point<float> evalDeBoor(double tt) const;

    // Hit-test against the currently active point set
    static constexpr float kHitR = 10.0f;
    int pickPoint(juce::Point<float> pos) const;

    // Helpers
    void drawPolyline(juce::Graphics& g,
                      const std::vector<juce::Point<float>>& pts,
                      juce::Colour colour, float thickness, bool dashed = false) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeBoorPolyCanvas)
};
