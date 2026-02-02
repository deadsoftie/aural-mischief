#pragma once
#include "IProject.h"
#include "Project1Component.h"

class Project1 final : public IProject
{
public:
	juce::String getName() const override { return "Project 1 - NLI & BB"; }

	std::unique_ptr<juce::Component> createView() override
	{
		return std::make_unique<Project1Component>();
	}
};
