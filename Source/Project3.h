#pragma once
#include "IProject.h"
#include "Project3Component.h"

class Project3 final : public IProject
{
public:
	juce::String getName() const override { return "Project 3 - Interpolating Polynomials"; }

	std::unique_ptr<juce::Component> createView() override
	{
		return std::make_unique<Project3Component>();
	}
};
