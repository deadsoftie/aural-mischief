#pragma once
#include <JuceHeader.h>

class CubicSplineCanvas : public juce::Component
{
public:
    CubicSplineCanvas();
    void setSegments(int k);
    void clearPoints();

    void paint(juce::Graphics &g) override;
    void mouseDown(const juce::MouseEvent &e) override;
    void mouseDrag(const juce::MouseEvent &e) override;
    void mouseUp(const juce::MouseEvent &e) override;

private:
    int maxSegments = 3; // k; max points = k+1
    std::vector<juce::Point<float>> points;
    int dragIndex = -1;

    static constexpr float hitRadius = 10.0f;
    int pickPoint(juce::Point<float> mousePos) const;
    std::vector<double> coeffX, coeffY;

    void rebuildSpline();

    // Gaussian elimination with partial pivoting; returns false if singular
    static bool gaussElim(std::vector<std::vector<double>> &A,
                          std::vector<double> &rhs,
                          std::vector<double> &sol);

    static double evalSpline(double t, const std::vector<double> &c, int k);

    void buildCurvePolyline(std::vector<juce::Point<float>> &out) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(CubicSplineCanvas)
};
