#include "NewtonInterpCanvas.h"

NewtonInterpCanvas::NewtonInterpCanvas()
{
	setWantsKeyboardFocus(true);
}

void NewtonInterpCanvas::setDegree(int newDegree)
{
	degree = juce::jlimit(1, 20, newDegree);

	const int maxPts = degree + 1;
	if (static_cast<int>(points.size()) > maxPts)
		points.resize(static_cast<size_t>(maxPts));

	rebuildAll();
	repaint();
}

void NewtonInterpCanvas::clearPoints()
{
	points.clear();
	dragIndex = -1;
	coeffX.clear();
	coeffY.clear();
	basePoints.clear();
	repaint();
}

int NewtonInterpCanvas::pickPoint(juce::Point<float> mousePos) const
{
	const float r2 = hitRadius * hitRadius;
	for (int i = 0; i < static_cast<int>(points.size()); ++i)
	{
		const auto& p = points[static_cast<size_t>(i)].p;
		if (p.getDistanceSquaredFrom(mousePos) <= r2)
			return i;
	}
	return -1;
}

void NewtonInterpCanvas::buildNewtonCoeffsFromPoints(
	const std::vector<SamplePoint>& pts,
	std::vector<double>& outCoeffX,
	std::vector<double>& outCoeffY)
{
	const int n = static_cast<int>(pts.size());
	outCoeffX.assign(static_cast<size_t>(n), 0.0);
	outCoeffY.assign(static_cast<size_t>(n), 0.0);

	if (n == 0) return;

	// Standard O(n^2) divided differences using a work array:
	// dd[i] holds current column values; we store first element of each column as coefficient.
	std::vector<double> ddX(static_cast<size_t>(n)), ddY(static_cast<size_t>(n));
	for (int i = 0; i < n; ++i)
	{
		ddX[static_cast<size_t>(i)] = static_cast<double>(pts[static_cast<size_t>(i)].p.x); // g_x(t_i) = x_i
		ddY[static_cast<size_t>(i)] = static_cast<double>(pts[static_cast<size_t>(i)].p.y); // g_y(t_i) = y_i
	}

	outCoeffX[0] = ddX[0];
	outCoeffY[0] = ddY[0];

	// t_i = i
	for (int k = 1; k < n; ++k)
	{
		for (int i = 0; i < n - k; ++i)
		{
			const double denom = pts[static_cast<size_t>(i + k)].t - pts[static_cast<size_t>(i)].t;
			ddX[static_cast<size_t>(i)] = (ddX[static_cast<size_t>(i + 1)] - ddX[static_cast<size_t>(i)]) / denom;
			ddY[static_cast<size_t>(i)] = (ddY[static_cast<size_t>(i + 1)] - ddY[static_cast<size_t>(i)]) / denom;
		}
		outCoeffX[static_cast<size_t>(k)] = ddX[0];
		outCoeffY[static_cast<size_t>(k)] = ddY[0];
	}
}

double NewtonInterpCanvas::evalNewton(
	double t,
	const std::vector<SamplePoint>& pts,
	const std::vector<double>& coeffs)
{
	const int n = static_cast<int>(coeffs.size());
	if (n == 0) return 0.0;

	// p(t) = a0 + (t-0)(a1 + (t-1)(a2 + ...))
	double acc = coeffs[static_cast<size_t>(n - 1)];
	for (int k = n - 2; k >= 0; --k)
		acc = coeffs[static_cast<size_t>(k)] + (t - pts[static_cast<size_t>(k)].t) * acc;

	return acc;
}

void NewtonInterpCanvas::rebuildAll()
{
	buildNewtonCoeffsFromPoints(points, coeffX, coeffY);
}

void NewtonInterpCanvas::rebuildBaseWithoutIndex(int idx)
{
	basePoints.clear();
	basePoints.reserve(!points.empty() ? points.size() - 1 : 0);

	for (int i = 0; i < static_cast<int>(points.size()); ++i)
		if (i != idx)
			basePoints.push_back(points[static_cast<size_t>(i)]);
}

void NewtonInterpCanvas::rebuildFromBasePlusLast(const SamplePoint& lastPt)
{
	std::vector<SamplePoint> tmp = basePoints;
	tmp.push_back(lastPt);

	buildNewtonCoeffsFromPoints(tmp, coeffX, coeffY);
}

void NewtonInterpCanvas::buildCurvePolyline(std::vector<juce::Point<float>>& out) const
{
	out.clear();

	const bool isDragging = (dragIndex >= 0);
	std::vector<SamplePoint> evalPts;

	if (dragUsingIncremental && isDragging)
	{
		// Use the exact same order used to build coeffs during dragging: basePoints + dragged point last
		evalPts = basePoints;
		evalPts.push_back(points[static_cast<size_t>(dragIndex)]);
	}
	else
	{
		evalPts = points;
	}


	const int n = static_cast<int>(points.size());
	if (n < 2) return;

	const int d = n - 1;            // interval [0,d]

	constexpr int samples = 700;
	out.reserve(static_cast<size_t>(samples) + 1);

	for (int s = 0; s <= samples; ++s)
	{
		const double t = static_cast<double>(d) * static_cast<double>(s) / static_cast<double>(samples);

		const float x = static_cast<float>(evalNewton(t, evalPts, coeffX));
		const float y = static_cast<float>(evalNewton(t, evalPts, coeffY));

		out.emplace_back(x, y);
	}
}

void NewtonInterpCanvas::mouseUp(const juce::MouseEvent& e)
{
	dragIndex = -1;

	// After drag ends, rebuild with the real ordering (simple + consistent)
	rebuildAll();
	repaint();
}

void NewtonInterpCanvas::mouseDown(const juce::MouseEvent& e)
{
	dragIndex = pickPoint(e.position);

	// Click empty space -> add point (until d+1 points)
	if (dragIndex < 0)
	{
		const int maxPts = degree + 1;
		if (static_cast<int>(points.size()) < maxPts)
		{
			const double tNew = static_cast<double>(points.size()); // t_i = i at creation time
			points.push_back({ e.position, tNew });

			rebuildAll();
			repaint();
		}
		return;
	}

	// Click on existing point -> begin drag.
	if (dragUsingIncremental)
		rebuildBaseWithoutIndex(dragIndex);
}

void NewtonInterpCanvas::mouseDrag(const juce::MouseEvent& e)
{
	if (dragIndex < 0) return;

	points[static_cast<size_t>(dragIndex)].p = e.position;

	if (dragUsingIncremental)
	{
		// recompute from base + current dragged point as last
		rebuildFromBasePlusLast(points[static_cast<size_t>(dragIndex)]);
	}
	else
	{
		rebuildAll();
	}

	repaint();
}

void NewtonInterpCanvas::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey.darker(0.7f));

	// Instruction text
	if (static_cast<int>(points.size()) < degree + 1)
	{
		g.setColour(juce::Colours::white.withAlpha(0.7f));
		g.setFont(16.0f);
		g.drawText("Click to add interpolation points (" +
			juce::String(points.size()) + "/" + juce::String(degree + 1) + ")",
			getLocalBounds().reduced(10),
			juce::Justification::topLeft);
	}

	// Draw polyline through points
	if (points.size() >= 2)
	{
		juce::Path poly;
		poly.startNewSubPath(points[0].p);
		for (size_t i = 1; i < points.size(); ++i)
			poly.lineTo(points[i].p);

		g.setColour(juce::Colours::orange.withAlpha(0.75f));
		g.strokePath(poly, juce::PathStrokeType(2.0f));
	}

	// Draw interpolating curve
	if (points.size() >= 2)
	{
		std::vector<juce::Point<float>> curve;
		buildCurvePolyline(curve);

		if (curve.size() >= 2)
		{
			juce::Path path;
			path.startNewSubPath(curve[0]);
			for (size_t i = 1; i < curve.size(); ++i)
				path.lineTo(curve[i]);

			g.setColour(juce::Colours::white);
			g.strokePath(path, juce::PathStrokeType(2.5f));
		}
	}

	// Draw points + labels
	for (int i = 0; i < static_cast<int>(points.size()); ++i)
	{
		const auto pt = points[static_cast<size_t>(i)].p;
		g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
		g.fillEllipse(pt.x - 6.0f, pt.y - 6.0f, 12.0f, 12.0f);

		g.setColour(juce::Colours::black.withAlpha(0.6f));
		g.drawText("P" + juce::String(i),
			static_cast<int>(pt.x) + 8, static_cast<int>(pt.y) - 8, 40, 16,
			juce::Justification::left);
	}
}