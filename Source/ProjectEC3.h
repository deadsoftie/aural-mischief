#pragma once
#include "IProject.h"
#include "ProjectEC3Component.h"

class ProjectEC3 final : public IProject
{
public:
    juce::String getName() const override { return "EC-3 - Audio BB Polynomial"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<ProjectEC3Component>();
    }
};
