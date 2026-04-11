#pragma once
#include "IProject.h"
#include "Project6Component.h"

class Project6 final : public IProject
{
public:
    juce::String getName() const override { return "Project 6 - De Boor (Polynomial Curves)"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<Project6Component>();
    }
};
