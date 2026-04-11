#pragma once
#include "IProject.h"
#include "ProjectEC2Component.h"

class ProjectEC2 final : public IProject
{
public:
    juce::String getName() const override { return "EC-2 - Best Fit Line & Parabola"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<ProjectEC2Component>();
    }
};
