#pragma once
#include <JuceHeader.h>

class NewtonInterpCanvas : public juce::Component
{
public:
	NewtonInterpCanvas();

	void setDegree(int newDegree);
	void clearPoints();

	void paint(juce::Graphics& g) override;
	void mouseDown(const juce::MouseEvent& e) override;
	void mouseDrag(const juce::MouseEvent& e) override;
	void mouseUp(const juce::MouseEvent& e) override;
private:
	struct SamplePoint
	{
		juce::Point<float> p;
		double t;
	};

	// model
	int degree = 3; // target d (needs d+1 points)
	std::vector<SamplePoint> points; // interpolation points P_i = (a_i,b_i)
	int dragIndex = -1;

	static constexpr float hitRadius = 10.0f;
	int pickPoint(juce::Point<float> mousePos) const;

	std::vector<double> coeffX, coeffY;

	// keep a "base" set without the dragged point, then we append the dragged point as last
	bool dragUsingIncremental = true;
	std::vector<SamplePoint> basePoints;

	void rebuildAll();
	void rebuildBaseWithoutIndex(int idx); // base coefficients for points excluding idx
	void rebuildFromBasePlusLast(const SamplePoint& lastPt); // incremental add last point

	static void buildNewtonCoeffsFromPoints(
		const std::vector<SamplePoint>& pts,
		std::vector<double>& outCoeffX,
		std::vector<double>& outCoeffY);

	static double evalNewton(
		double t,
		const std::vector<SamplePoint>& pts,
		const std::vector<double>& coeffs);

	void buildCurvePolyline(std::vector<juce::Point<float>>& out) const;
};