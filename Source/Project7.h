#pragma once
#include "IProject.h"
#include "Project7Component.h"

class Project7 final : public IProject
{
public:
    juce::String getName() const override { return "Project 7 - De Boor (B-Spline Curves)"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<Project7Component>();
    }
};
