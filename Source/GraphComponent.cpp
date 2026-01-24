#include "GraphComponent.h"

static constexpr float plotPaddingLeft = 40.0f;
static constexpr float plotPaddingRight = 30.0f;
static constexpr float plotPaddingTop = 30.0f;
static constexpr float plotPaddingBottom = 40.0f;

GraphComponent::GraphComponent()
{
	setWantsKeyboardFocus(true);
}

void GraphComponent::setModel(int newDegree, std::vector<double>* newCoeffs, EvalMethod newMethod, const std::vector<std::vector<double>>* newChoose)
{
	degree = newDegree;
	coeffs = newCoeffs;
	method = newMethod;
	choose = newChoose;
	repaint();
}

void GraphComponent::setMethod(EvalMethod newMethod)
{
	method = newMethod;
	repaint();
}

double GraphComponent::eval(double t) const
{
	if (coeffs == nullptr) return 0.0;

	const auto& a = *coeffs;

	if (degree <= 0) return a.empty() ? 0.0 : a[0];

	if (method == EvalMethod::NLI)
	{
		// De Casteljau / NLI

		std::vector<double> tmp = a; // for up to d = 20 this is more than enough

		for (int r = 1; r <= degree; ++r)
		{
			for (int i = 0; i <= degree - r; ++i)
			{
				tmp[static_cast<size_t>(i)] = (1.0 - t) * tmp[static_cast<size_t>(i)] + t * tmp[static_cast<size_t>(i) + 1];
			}
		}

		return tmp[0];
	}

	// BB-form
	if (choose == nullptr) return 0.0;

	const double u = 1.0 - t;
	double sum = 0.0;

	for (int i = 0; i <= degree; ++i)
	{
		const double c = (*choose)[static_cast<size_t>(degree)][static_cast<size_t>(i)];
		const double basis = c * std::pow(u, degree - i) * std::pow(t, i);
		sum += a[static_cast<size_t>(i)] * basis;
	}

	return sum;
}

// Helper function that will allow re-sizing everywhere without breaking inputs
juce::Rectangle<float> GraphComponent::getPlotRect() const
{
	auto bounds = getLocalBounds().toFloat();

	return bounds.withTrimmedLeft(plotPaddingLeft)
		.withTrimmedRight(plotPaddingRight)
		.withTrimmedTop(plotPaddingTop)
		.withTrimmedBottom(plotPaddingBottom);
}

juce::Point<float> GraphComponent::worldToScreen(double xW, double yW, juce::Rectangle<float> plot)
{
	const float x = static_cast<float>(juce::jmap(xW, xMin, xMax, static_cast<double>(plot.getX()), static_cast<double>(plot.getRight())));
	const float y = static_cast<float>(juce::jmap(yW, yMin, yMax, static_cast<double>(plot.getBottom()), static_cast<double>(plot.getY()))); // invert y

	return { x,y };
}

double GraphComponent::screenToWorldY(float yScreen, juce::Rectangle<float> plot)
{
	return juce::jmap(static_cast<double>(yScreen), static_cast<double>(plot.getBottom()), static_cast<double>(plot.getY()), yMin, yMax);
}

int GraphComponent::pickPoint(juce::Point<float> mousePos, juce::Rectangle<float> plot) const
{
	if (coeffs == nullptr) return -1;

	const float radius = 9.0f;
	const float r2 = radius * radius;

	const auto& a = *coeffs;

	for (int i = 0; i <= degree; ++i)
	{
		const double t = (degree == 0) ? 0.0 : static_cast<double>(i) / static_cast<double>(degree);
		const double xW = 6.0 * t;
		const auto p = worldToScreen(xW, a[static_cast<size_t>(i)], plot);

		if (p.getDistanceSquaredFrom(mousePos) <= r2) return i;
	}

	return -1;
}

void GraphComponent::drawAxes(juce::Graphics& g, juce::Rectangle<float> plot)
{
	g.setColour(juce::Colours::grey);
	g.drawRect(plot, 1.0f);

	g.setColour(juce::Colours::darkgrey);

	// x-axis
	{
		const auto p0 = worldToScreen(xMin, 0.0, plot);
		const auto p1 = worldToScreen(xMax, 0.0, plot);
		g.drawLine({ p0.x, p0.y, p1.x, p1.y }, 1.0f);
	}

	// y-axis
	{
		const auto p0 = worldToScreen(0.0, yMin, plot);
		const auto p1 = worldToScreen(0.0, yMax, plot);
		g.drawLine({ p0.x, p0.y, p1.x, p1.y }, 1.0f);
	}

	const float xAxisY = worldToScreen(0.0, 0.0, plot).y;

	constexpr int labelH = 16;
	const int yLabel = static_cast<int>(std::round(xAxisY + 2.0f));

	// graph labels
	g.setColour(juce::Colours::lightgrey);
	g.setFont(15.0f);
	g.drawText("t = 0", static_cast<int>(plot.getX()) + 2, yLabel, 40, labelH, juce::Justification::left);
	g.drawText("t = 1", static_cast<int>(plot.getRight()) - 42, yLabel, 40, labelH, juce::Justification::right);
	g.drawText("y = 3", static_cast<int>(plot.getX()) + 2, static_cast<int>(plot.getY()) - 18, 40, 16, juce::Justification::left);
	g.drawText("y = -3", static_cast<int>(plot.getX()) + 2, static_cast<int>(plot.getBottom()) + 2, 40, 16, juce::Justification::left);
}

void GraphComponent::drawCurve(juce::Graphics& g, juce::Rectangle<float> plot) const
{
	juce::Path path;

	constexpr int samples = 600;

	for (int s = 0; s <= samples; ++s)
	{
		const double t = static_cast<double>(s) / static_cast<double>(samples);
		double y = eval(t);
		y = juce::jlimit(yMin, yMax, y);

		const auto p = worldToScreen(6.0 * t, y, plot);
		if (s == 0) path.startNewSubPath(p);
		else path.lineTo(p);
	}

	g.setColour(juce::Colours::white);
	g.strokePath(path, juce::PathStrokeType(2.0f));
}

void GraphComponent::drawControlPoints(juce::Graphics& g, juce::Rectangle<float> plot) const
{
	if (coeffs == nullptr) return;

	const auto& a = *coeffs;
	for (int i = 0; i <= degree; ++i)
	{
		const double t = (degree == 0) ? 0.0 : static_cast<double>(i) / static_cast<double>(degree);
		const auto p = worldToScreen(6.0 * t, a[static_cast<size_t>(i)], plot);

		g.setColour(i == dragIndex ? juce::Colours::yellow : juce::Colours::cyan);
		g.fillEllipse(p.x - 5.0f, p.y - 5.0f, 10.0f, 10.0f);
	}
}

void GraphComponent::paint(juce::Graphics& g)
{
	g.fillAll(juce::Colours::black);

	auto plot = getPlotRect();

	drawAxes(g, plot);
	drawCurve(g, plot);
	drawControlPoints(g, plot);
}

void GraphComponent::mouseDown(const juce::MouseEvent& event)
{
	auto plot = getPlotRect();
	dragIndex = pickPoint(event.position, plot);
}

void GraphComponent::mouseDrag(const juce::MouseEvent& event)
{
	if (dragIndex < 0 || coeffs == nullptr) return;

	auto plot = getPlotRect();
	double yWorld = screenToWorldY(event.position.y, plot);
	yWorld = juce::jlimit(yMin, yMax, yWorld);
	(*coeffs)[static_cast<size_t>(dragIndex)] = yWorld;

	repaint();
}

void GraphComponent::mouseUp(const juce::MouseEvent& event)
{
	dragIndex = -1;
}
