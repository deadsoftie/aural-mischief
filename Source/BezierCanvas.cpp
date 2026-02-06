#include "BezierCanvas.h"

BezierCanvas::BezierCanvas()
{
	buildChooseTable(20);
	setWantsKeyboardFocus(true);
}

void BezierCanvas::setDegree(int newDegree)
{
	degree = juce::jlimit(1, 20, newDegree);

	// clamp existing points
	const int maxPoints = degree + 1;
	if (static_cast<int>(controlPoints.size()) > maxPoints)
		controlPoints.resize(static_cast<size_t>(maxPoints));

	repaint();
}

void BezierCanvas::setMethod(Method newMethod)
{
	method = newMethod;
	repaint();
}

void BezierCanvas::setT(double newT)
{
	t = juce::jlimit(0.0, 1.0, newT);
	repaint();
}

void BezierCanvas::clearPoints()
{
	controlPoints.clear();
	dragIndex = -1;
	repaint();
}

void BezierCanvas::resetToDefaultIfEmpty()
{
	if (!controlPoints.empty())
		return;

	// Default layout
	auto bounds = getLocalBounds().toFloat().reduced(60.0f);
	controlPoints.emplace_back(bounds.getX(), bounds.getCentreY() + 80.0f);
	controlPoints.emplace_back(bounds.getCentreX(), bounds.getY());
	controlPoints.emplace_back(bounds.getRight(), bounds.getCentreY() + 80.0f);
}

void BezierCanvas::buildChooseTable(int maxDegree)
{
	choose.assign(static_cast<size_t>(maxDegree) + 1, std::vector<double>(static_cast<size_t>(maxDegree) + 1, 0.0));
	for (int n = 0; n <= maxDegree; ++n)
	{
		choose[static_cast<size_t>(n)][0] = 1.0;
		choose[static_cast<size_t>(n)][static_cast<size_t>(n)] = 1.0;
		for (int k = 1; k < n; ++k)
			choose[static_cast<size_t>(n)][static_cast<size_t>(k)] =
			choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k - 1)] + choose[static_cast<size_t>(n - 1)][static_cast<size_t>(k)];
	}
}

int BezierCanvas::pickPoint(juce::Point<float> point) const
{
	constexpr float r2 = hitRadius * hitRadius;
	for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i)
		if (controlPoints[static_cast<size_t>(i)].getDistanceSquaredFrom(point) <= r2)
			return i;
	return -1;
}

juce::Point<float> BezierCanvas::evalNLI(double tt,
	std::vector<std::vector<juce::Point<float>>>* outTriangle) const
{
	const int n = static_cast<int>(controlPoints.size());
	if (n == 0) return {};
	if (n == 1) return controlPoints[0];

	const int dEff = n - 1;

	std::vector<std::vector<juce::Point<float>>> tri;
	tri.resize(static_cast<size_t>(dEff) + 1);
	tri[0] = controlPoints; // size n

	for (int r = 1; r <= dEff; ++r)
	{
		tri[static_cast<size_t>(r)].resize(static_cast<size_t>(dEff - r + 1));
		for (int i = 0; i <= dEff - r; ++i)
		{
			auto a = tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i)];
			auto b = tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i) + 1];
			tri[static_cast<size_t>(r)][static_cast<size_t>(i)] = a * static_cast<float>(1.0 - tt) + b * static_cast<float>(tt);
		}
	}

	if (outTriangle) *outTriangle = tri;
	return tri[static_cast<size_t>(dEff)][0];
}


juce::Point<float> BezierCanvas::evalBB(double tt) const
{
	const int n = static_cast<int>(controlPoints.size());
	if (n == 0) return {};
	if (n == 1) return controlPoints[0];

	const int dEff = n - 1;

	const double u = 1.0 - tt;
	juce::Point<double> sum{ 0.0, 0.0 };

	for (int i = 0; i <= dEff; ++i)
	{
		const double c = choose[static_cast<size_t>(dEff)][static_cast<size_t>(i)];
		const double basis = c * std::pow(u, dEff - i) * std::pow(tt, i);

		const auto& p = controlPoints[static_cast<size_t>(i)];
		sum += juce::Point<double>((double)p.x, (double)p.y) * basis;
	}

	return { static_cast<float>(sum.x), static_cast<float>(sum.y) };
}


void BezierCanvas::buildPolyLine(std::vector<juce::Point<float>>& out) const
{
	out.clear();
	if (controlPoints.size() < 2)
		return;

	switch (method)
	{
	case Method::NLI:
		buildPolyLineNLIMethod(out);
		break;
	case Method::BB:
		buildPolyLineBBMethod(out);
		break;
	case Method::Midpoint:
		buildPolyLineMidpointMethod(out);
		break;
	}
}

void BezierCanvas::buildPolyLineNLIMethod(std::vector<juce::Point<float>>& out) const
{
	constexpr int samples = 600;
	out.reserve(samples + 1);

	for (int s = 0; s <= samples; ++s)
	{
		const double tt = static_cast<double>(s) / static_cast<double>(samples);
		out.push_back(evalNLI(tt, nullptr));
	}
}

void BezierCanvas::buildPolyLineBBMethod(std::vector<juce::Point<float>>& out) const
{
	constexpr int samples = 600;
	out.reserve(samples + 1);

	for (int s = 0; s <= samples; ++s)
	{
		const double tt = static_cast<double>(s) / static_cast<double>(samples);
		out.push_back(evalBB(tt));
	}
}

void BezierCanvas::subdivideMidpoint(const std::vector<juce::Point<float>>& points,
	int depth,
	std::vector<juce::Point<float>>& out) const
{
	if (depth <= 0)
	{
		if (out.empty()) out.push_back(points.front());
		for (size_t i = 1; i < points.size(); ++i) out.push_back(points[i]);
		return;
	}

	// Build triangle at t=0.5
	std::vector<std::vector<juce::Point<float>>> tri;
	tri.resize(points.size());
	tri[0] = points;

	const int dLocal = static_cast<int>(points.size()) - 1;

	for (int r = 1; r <= dLocal; ++r)
	{
		tri[static_cast<size_t>(r)].resize(static_cast<size_t>(dLocal - r + 1));
		for (int i = 0; i <= dLocal - r; ++i)
		{
			auto a = tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i)];
			auto b = tri[static_cast<size_t>(r - 1)][static_cast<size_t>(i) + 1];
			tri[static_cast<size_t>(r)][static_cast<size_t>(i)] = a * 0.5f + b * 0.5f;
		}
	}

	// Extract left and right control polygons from triangle edges
	std::vector<juce::Point<float>> left, right;
	left.reserve(points.size());
	right.reserve(points.size());

	for (int r = 0; r <= dLocal; ++r)
		left.push_back(tri[static_cast<size_t>(r)][0]);

	for (int r = dLocal; r >= 0; --r)
		right.push_back(tri[static_cast<size_t>(r)][static_cast<size_t>(dLocal - r)]);

	// Recurse
	subdivideMidpoint(left, depth - 1, out);
	if (!out.empty())
		out.pop_back();
	subdivideMidpoint(right, depth - 1, out);
}

void BezierCanvas::buildPolyLineMidpointMethod(std::vector<juce::Point<float>>& out) const
{
	if (controlPoints.size() < 2) return;
	constexpr int depth = 6;
	subdivideMidpoint(controlPoints, depth, out);
}

void BezierCanvas::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::darkgrey.darker(0.7f));

	// draw instructions if not enough points
	if (static_cast<int>(controlPoints.size()) < degree + 1)
	{
		g.setColour(juce::Colours::white.withAlpha(0.7f));
		g.setFont(16.0f);
		g.drawText("Click to add control points (" + juce::String(controlPoints.size()) + "/" + juce::String(degree + 1) + ")",
			getLocalBounds().reduced(10), juce::Justification::topLeft);
	}

	// control line
	if (controlPoints.size() >= 2)
	{
		juce::Path poly;
		poly.startNewSubPath(controlPoints[0]);
		for (size_t i = 1; i < controlPoints.size(); ++i)
			poly.lineTo(controlPoints[i]);

		g.setColour(juce::Colours::orange.withAlpha(0.9f));
		g.strokePath(poly, juce::PathStrokeType(2.0f));
	}

	// curve polyline (as soon as we have >= 2 points)
	if (controlPoints.size() >= 2)
	{
		std::vector<juce::Point<float>> curve;
		buildPolyLine(curve);

		if (curve.size() >= 2)
		{
			juce::Path path;
			path.startNewSubPath(curve[0]);
			for (size_t i = 1; i < curve.size(); ++i)
				path.lineTo(curve[i]);

			g.setColour(juce::Colours::white);
			g.strokePath(path, juce::PathStrokeType(2.5f));
		}

		// shells only for NLI (and only meaningful if >=2 points)
		if (method == Method::NLI)
		{
			std::vector<std::vector<juce::Point<float>>> tri;
			const auto pt = evalNLI(t, &tri);

			const int dEff = static_cast<int>(controlPoints.size()) - 1;

			g.setColour(juce::Colours::cyan.withAlpha(0.25f));
			for (int r = 0; r < dEff; ++r)
			{
				for (int i = 0; i < dEff - r; ++i)
				{
					auto a = tri[static_cast<size_t>(r)][static_cast<size_t>(i)];
					auto b = tri[static_cast<size_t>(r)][static_cast<size_t>(i) + 1];
					g.drawLine({ a.x, a.y, b.x, b.y }, 1.5f);
				}
			}

			g.setColour(juce::Colours::cyan.withAlpha(0.6f));
			for (int r = 0; r <= dEff; ++r)
				for (int i = 0; i <= dEff - r; ++i)
					g.fillEllipse(tri[static_cast<size_t>(r)][static_cast<size_t>(i)].x - 3.0f,
						tri[static_cast<size_t>(r)][static_cast<size_t>(i)].y - 3.0f,
						6.0f, 6.0f);

			g.setColour(juce::Colours::yellow);
			g.fillEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
		}
		else
		{
			// moving point for BB / Midpoint
			juce::Point<float> pt = (method == Method::BB) ? evalBB(t) : evalNLI(t, nullptr);
			g.setColour(juce::Colours::yellow);
			g.fillEllipse(pt.x - 5.0f, pt.y - 5.0f, 10.0f, 10.0f);
		}
	}


	// control points
	for (int i = 0; i < static_cast<int>(controlPoints.size()); ++i)
	{
		const auto p = controlPoints[static_cast<size_t>(i)];
		g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
		g.fillEllipse(p.x - 6.0f, p.y - 6.0f, 12.0f, 12.0f);

		g.setColour(juce::Colours::black.withAlpha(0.6f));
		g.drawText("P" + juce::String(i), static_cast<int>(p.x) + 8, static_cast<int>(p.y) - 8, 40, 16, juce::Justification::left);
	}
}

void BezierCanvas::mouseDown(const juce::MouseEvent& event)
{
	dragIndex = pickPoint(event.position);

	if (dragIndex < 0)
	{
		const int maxPoints = degree + 1;
		if (static_cast<int>(controlPoints.size()) < maxPoints)
		{
			controlPoints.emplace_back(event.position);
			repaint();
		}
	}
}

void BezierCanvas::mouseDrag(const juce::MouseEvent& event)
{
	if (dragIndex < 0) return;
	controlPoints[static_cast<size_t>(dragIndex)] = event.position;
	repaint();
}

void BezierCanvas::mouseUp(const juce::MouseEvent& event)
{
	dragIndex = -1;
}
