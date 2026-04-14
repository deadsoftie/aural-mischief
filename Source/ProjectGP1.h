#pragma once
#include "IProject.h"
#include "ProjectGP1Component.h"

class ProjectGP1 final : public IProject
{
public:
    juce::String getName() const override { return "GP-1 - Catmull-Rom Splines"; }
    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<ProjectGP1Component>();
    }
};
