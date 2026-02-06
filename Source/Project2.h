#pragma once
#include "IProject.h"
#include "Project2Component.h"

class Project2 final : public IProject
{
public:
	juce::String getName() const override { return "Project 2 - Bezier Curves"; }

	std::unique_ptr<juce::Component> createView() override
	{
		return std::make_unique<Project2Component>();
	}
};
