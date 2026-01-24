# Project 1 - De Casteljau Algorithm for Polynomial Functions

## Overview

This project is an interactive C++ application built using the JUCE framework to visualize and evaluate polynomial functions expressed in the Bernstein basis.

The program allows users to:

- Change the polynomial degree (up to 20)
- Interactively modify Bernstein coefficients using draggable control points
- Evaluate the polynomial using two equivalent methods:
  - Nested Linear Interpolation (De Casteljau’s Algorithm)
  - Bernstein Basis (BB-form) Sum

Both methods produce the same curve and are provided for comparison and verification.

## Running the Project

Double-click the `.exe` file to launch the application.

## Building the Project

Using the `Source` folder that is provided just generate a project using the JUCE Projucer and run the project from any IDEs.

## Source Code Structure

- `Main.cpp`
  - Application entry point.
  - Initializes the JUCE application and main window.

- `MainComponent.h / MainComponent.cpp`
  - Manages the user interface and application state.
  - Owns:
    - Polynomial degree
    - Co-efficient vector
    - Precomputed binomial co-efficient table
  - Handles:
    - Degree slider changes
    - Method selection
    - Reset behavior
  - Passes model data to `GraphComponent`

- `GraphComponent.h / GraphComponent.cpp`
  - Responsible for rendering and interaction.
  - Draws:
    - Grid, axes, and labels
    - Polynomial curve
    - Draggable control points
  - Implements both evaluation methods:
    - De Casteljau Algorithm (NLI)
    - Bernstein Basis (BB-form)
  - Handles mouse input for selecting and dragging control points.

## AI Assistance Disclosure

AI was used for code review and it identified an edge case in the Bernstein Sum evaluation (endpoint handling). It helped in decluttering the UI by suggesting the use of spacers between the controls and recommended the use of a slider instead of a dropdown (was using a `ComboBox` earlier).
