#pragma once
#include <JuceHeader.h>

class CatmullRomCanvas : public juce::Component
{
public:
    enum class Parameterization { Uniform, Centripetal, Chordal };

    CatmullRomCanvas();

    void setParameterization(Parameterization p);
    void setShowTangents(bool show);
    void setShowBezierEquivalent(bool show);
    void setExtendToEndpoints(bool extend);
    void clearPoints();

    int getSegmentCount() const;

    std::function<void()> onPointsChanged;

    void paint(juce::Graphics& g) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;

private:
    std::vector<juce::Point<float>> pts;
    Parameterization param = Parameterization::Centripetal;
    bool showTangents = false;
    bool showBezierEquiv = false;
    bool extendToEndpoints = false;
    int dragIndex = -1;

    static constexpr float hitRadius = 10.0f;
    int pickPoint(juce::Point<float> pos) const;

    // Returns alpha for the current parameterization
    double getAlpha() const;

    // Builds the knot sequence for given points and alpha
    static std::vector<double> buildKnots(const std::vector<juce::Point<float>>& p, double alpha);

    // Barry-Goldman NLI: evaluate segment [t1,t2] with four pts P0..P3 and knots t0..t3
    static juce::Point<float> evalBarryGoldman(
        juce::Point<float> P0, juce::Point<float> P1,
        juce::Point<float> P2, juce::Point<float> P3,
        double t0, double t1, double t2, double t3,
        double u);

    // Builds the full polyline for drawing
    void buildCurve(std::vector<juce::Point<float>>& out) const;

    // Computes tangent at point index i using knots
    static juce::Point<float> computeTangent(
        const std::vector<juce::Point<float>>& p,
        const std::vector<double>& knots,
        int i);

    // Draws a line with dashes
    void drawDashedLine(juce::Graphics& g,
        juce::Point<float> a, juce::Point<float> b,
        juce::Colour colour, float thickness) const;

    // Draws an arrow (line + arrowhead)
    void drawArrow(juce::Graphics& g,
        juce::Point<float> origin, juce::Point<float> tip,
        juce::Colour colour, float thickness) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CatmullRomCanvas)
};
