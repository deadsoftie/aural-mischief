#pragma once
#include <JuceHeader.h>

class BestFitCanvas : public juce::Component
{
public:
    BestFitCanvas();

    void setMode(bool parabola);
    void clearPoints();

    float getLineCost()     const noexcept { return lineCost; }
    float getParabolaCost() const noexcept { return parabolaCost; }

    void paint   (juce::Graphics&)          override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp  (const juce::MouseEvent&) override;

private:
    std::vector<juce::Point<float>> pts;
    bool showParabola = false;
    int  dragIndex    = -1;

    // --- fit state ---
    juce::Point<float> M;           // mean of all pts
    juce::Point<float> lineFitDir;  // unit direction of Lbf
    float              lineCost     = 0.0f;
    juce::Point<float> bisDir;      // unit direction of B (perpendicular to Lbf through M)

    bool               hasSubMeans = false;
    juce::Point<float> M1, M2;     // sub-means (from winning family)

    bool               hasParabola  = false;
    juce::Point<float> Q0, Q1, Q2; // best-fit parabola Bezier control points
    float              parabolaCost = 0.0f;

    static constexpr int   kSamples       = 100;
    static constexpr int   kIter          = 3;
    static constexpr float kHitRadius     = 10.0f;
    static constexpr int   kBezierSamples = 50;

    void recompute();
    void fitLine();
    void fitParabola();

    float scoreLine   (float angle) const noexcept;
    float scoreBezier (juce::Point<float> q0, juce::Point<float> q1,
                       juce::Point<float> q2) const noexcept;

    static float perpDist    (juce::Point<float> p,
                               juce::Point<float> linePoint,
                               juce::Point<float> lineDir) noexcept;
    static float distToSegment(juce::Point<float> p,
                                juce::Point<float> a,
                                juce::Point<float> b) noexcept;
    static juce::Point<float> evalBezier(juce::Point<float> q0,
                                          juce::Point<float> q1,
                                          juce::Point<float> q2,
                                          float t) noexcept;
    static std::pair<juce::Point<float>, juce::Point<float>>
    clipLine(juce::Point<float> lp, juce::Point<float> ld, float w, float h);

    int pickPoint(juce::Point<float> pos) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BestFitCanvas)
};
