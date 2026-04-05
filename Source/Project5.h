/*
  ==============================================================================

    Project5.h
    Created: 4 Apr 2026 8:48:06pm
    Author:  rahul

  ==============================================================================
*/

#pragma once
#include "IProject.h"
#include "Project5Component.h"

class Project5 final : public IProject
{
public:
    juce::String getName() const override { return "Project 5 - B-Spline Functions (De Boor)"; }

    std::unique_ptr<juce::Component> createView() override
    {
        return std::make_unique<Project5Component>();
    }
};
