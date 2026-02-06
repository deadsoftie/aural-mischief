#pragma once
#include <JuceHeader.h>

class BezierCanvas : public juce::Component
{
public:
	enum class Method { NLI, BB, Midpoint };

	BezierCanvas();

	void setDegree(int newDegree);
	void setMethod(Method newMethod);
	void setT(double newT);

	void clearPoints();
	void resetToDefaultIfEmpty();

	void paint(juce::Graphics& g) override;

	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

private:
	// model
	int degree = 3; // degree default
	Method method = Method::NLI;
	double t = 0.5; // t-slider default

	std::vector<juce::Point<float>> controlPoints;
	int dragIndex = -1;

	// binomial coefficients choose[n][k], n<=20
	std::vector<std::vector<double>> choose;
	void buildChooseTable(int maxDegree);

	// drawing helpers
	static constexpr float hitRadius = 10.0f;
	int pickPoint(juce::Point<float> point) const;

	// evaluation
	juce::Point<float> evalNLI(double tt, std::vector<std::vector<juce::Point<float>>>* outTriangle) const;
	juce::Point<float> evalBB(double tt) const;

	// building the curve per method
	void buildPolyLine(std::vector<juce::Point<float>>& out) const;
	void buildPolyLineNLIMethod(std::vector<juce::Point<float>>& out) const;
	void buildPolyLineBBMethod(std::vector<juce::Point<float>>& out) const;
	void buildPolyLineMidpointMethod(std::vector<juce::Point<float>>& out) const;

	// midpoint subdivision
	void subdivideMidpoint(const std::vector<juce::Point<float>>& points, int depth, std::vector<juce::Point<float>>& out) const;

	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BezierCanvas)
};