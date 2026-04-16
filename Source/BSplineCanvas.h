#pragma once
#include <JuceHeader.h>

class BSplineCanvas : public juce::Component
{
public:
    BSplineCanvas();

    void setDegree(int d);
    void setT(double t);
    void clearPoints();
    void setKnotsFromString(const juce::String& s);

    juce::String getKnotString() const;
    double getTMin() const;
    double getTMax() const;
    bool hasValidCurve() const;

    std::function<void()> onStateChanged;

    void paint    (juce::Graphics& g)          override;
    void mouseDown(const juce::MouseEvent& e)  override;
    void mouseDrag(const juce::MouseEvent& e)  override;
    void mouseUp  (const juce::MouseEvent& e)  override;

private:
    int    degree   = 3;
    double tParam   = 0.0;
    int    dragIndex = -1;

    std::vector<juce::Point<float>> controlPts;
    std::vector<double>             knots;

    void buildDefaultKnots();
    int  findSpan(double t) const;
    std::vector<std::vector<juce::Point<float>>> computeShells(double t) const;
    juce::Point<float> evalBSpline(double t) const;

    int  pickPoint(juce::Point<float> pos) const;
    void drawPolyline(juce::Graphics& g,
                      const std::vector<juce::Point<float>>& pts,
                      juce::Colour col, float thickness) const;

    static constexpr float kHitR = 10.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BSplineCanvas)
};
