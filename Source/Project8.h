#pragma once
#include "IProject.h"
#include "Project8Component.h"

class Project8 final : public IProject
{
public:
    juce::String getName() const override { return "Project 8 - 3D Bezier Curves"; }
    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<Project8Component>();
    }
};
