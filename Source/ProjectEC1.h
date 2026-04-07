#pragma once
#include "IProject.h"
#include "ProjectEC1Component.h"

class ProjectEC1 final : public IProject
{
public:
    juce::String getName() const override { return "EC-1 - Hermite Interpolation"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<ProjectEC1Component>();
    }
};
