#pragma once
#include "IProject.h"
#include "Project4Component.h"

class Project4 final : public IProject
{
public:
    juce::String getName() const override { return "Project 4 - Interpolating Cubic Splines"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<Project4Component>();
    }
};
