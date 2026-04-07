/*
  ==============================================================================

    Project5Component.h
    Created: 4 Apr 2026 8:47:19pm
    Author:  rahul

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

// Plots a weighted sum of B-spline basis functions f(t) = sum c_i * B_i^d(t)
// over the uniform knot sequence t_k = k, for t in [d, N-d].
// Coefficient dots are draggable up/down, matching the structure of Project 1.
class Project5Component : public juce::Component
{
public:
    Project5Component();

    void paint   (juce::Graphics& g)            override;
    void resized ()                              override;
    void mouseDown(const juce::MouseEvent& e)   override;
    void mouseDrag(const juce::MouseEvent& e)   override;
    void mouseUp  (const juce::MouseEvent& e)   override;

private:
    // ── UI controls ──────────────────────────────────────────────────────────
    juce::Label      degreeLabel, nLabel;
    juce::Slider     degreeSlider, nSlider;
    juce::TextButton resetButton;

    // ── State ────────────────────────────────────────────────────────────────
    int    d         = 3;
    int    N         = 13;   // default = d + 10
    std::vector<double> coeffs;  // c_0 ... c_{N-d-1}, all start at 1.0
    int    dragIndex = -1;

    // ── Helpers ───────────────────────────────────────────────────────────────
    void   rebuild();                                   // resize coeffs, fill new entries with 1
    int    numCoeffs()      const { return N - d; }
    double dotXWorld(int i) const { return 0.5 * (d + 1) + static_cast<double>(i); }
    double xWorldMax()      const { return static_cast<double>(N); }

    // Evaluation
    double evalDeBoor(double t) const;

    // ── Layout constants ──────────────────────────────────────────────────────
    static constexpr float kPadL   = 50.0f;
    static constexpr float kPadR   = 30.0f;
    static constexpr float kPadT   = 30.0f;
    static constexpr float kPadB   = 50.0f;
    static constexpr int   kToolH  = 40;
    static constexpr int   kMargin = 8;

    // World y-bounds (fixed, matching Project 1)
    static constexpr double kYMin = -3.0;
    static constexpr double kYMax =  3.0;

    // ── Coordinate transforms ─────────────────────────────────────────────────
    juce::Rectangle<float> getPlotRect() const;
    juce::Point<float>     toScreen (double xW, double yW, juce::Rectangle<float> plot) const;
    double                 toWorldY (float ys,             juce::Rectangle<float> plot) const;
    int                    pickDot  (juce::Point<float> pos, juce::Rectangle<float> plot) const;

    // ── Drawing ───────────────────────────────────────────────────────────────
    void drawGrid         (juce::Graphics& g, juce::Rectangle<float> plot) const;

    void drawCurve        (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawDots         (juce::Graphics& g, juce::Rectangle<float> plot) const;
    void drawAxisLabels   (juce::Graphics& g, juce::Rectangle<float> plot) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Project5Component)
};
