#pragma once

#include <JuceHeader.h>

class GraphComponent : public juce::Component
{
public:
	enum class EvalMethod { NLI, BB };

	GraphComponent();

	void setModel(int newDegree, std::vector<double>* newCoeffs, EvalMethod newMethod, const std::vector<std::vector<double>>* newChoose);

	void setMethod(EvalMethod newMethod);

	void paint(juce::Graphics& g) override;

	void mouseDown(const juce::MouseEvent& event) override;
	void mouseDrag(const juce::MouseEvent& event) override;
	void mouseUp(const juce::MouseEvent& event) override;

private:
	int degree = 1;
	std::vector<double>* coeffs = nullptr; // owned in MainComponent
	EvalMethod method = EvalMethod::NLI;
	const std::vector<std::vector<double>>* choose = nullptr; // pascal triangle owned in MainComponent

	int dragIndex = -1;

	// World co-ordinates:
	// x in [0,6] where x = 6t so t in [0,1] maps to [0,6]
	// y in [-3,3]

	static constexpr double xMin = 0.0, xMax = 6.0;
	static constexpr double yMin = -3.0, yMax = 3.0;

	double eval(double t) const;

	juce::Rectangle<float> getPlotRect() const;

	static juce::Point<float> worldToScreen(double xW, double yW, juce::Rectangle<float> plot);
	static double screenToWorldY(float yScreen, juce::Rectangle<float> plot);

	int pickPoint(juce::Point<float> mousePos, juce::Rectangle<float> plot) const;

	static void drawAxes(juce::Graphics& g, juce::Rectangle<float> plot);
	void drawCurve(juce::Graphics& g, juce::Rectangle<float> plot) const;
	void drawControlPoints(juce::Graphics& g, juce::Rectangle<float> plot) const;
	void drawGrid(juce::Graphics& g, juce::Rectangle<float> plot) const;
};