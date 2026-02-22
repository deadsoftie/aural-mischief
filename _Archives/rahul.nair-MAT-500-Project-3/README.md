# Project 2 – Interactive Bézier Curve Visualization

Rahul Nair (rahul.nair@digipen.edu)

## Overview

This project is an interactive C++ application built using the JUCE framework to visualize parametric polynomial curves constructed through interpolation.

The application allows users to define interpolation points directly on the canvas and generates a parametric polynomial curve that passes exactly through those points using the Newton divided difference form.

## Running the Project

Double-click the `.exe` file to launch the application.

## Building the Project

Using the `Source` folder that is provided just generate a project using the JUCE Projucer and run the project from any IDEs.

## Architecture

- The application follows a modular, component-based structure similar to Project 2:
  - A UI/controller component manages user-facing controls such as:
  - Degree selection (up to 20)
  - Clear/reset functionality

- A dedicated rendering and interaction component encapsulates:
  - Interpolation point management and mouse interaction
  - Newton divided-difference coefficient construction
  - Parametric curve evaluation and drawing

Interpolation logic and rendering behavior are isolated from UI controls, allowing clean separation between user interaction, polynomial mathematics, and visualization.

## Features

- Interactive placement of interpolation points (up to degree 20)
- Real-time dragging of points with immediate curve update
- Parametric polynomial curve generation using Newton form
- Independent Newton interpolation for x(t) and y(t)
- Support for arbitrary node ordering during coefficient computation
- Efficient coefficient rebuilding during drag interaction
- Visual labeling of interpolation points
- Optional polyline display connecting interpolation points for reference

The curve is evaluated over the interval `[0,d]`, where `d` is the selected degree, ensuring consistency with the interpolation nodes.

## AI Assistance Disclosure

AI assistance was used to refine architectural decisions related to incremental coefficient recomputation and to streamline navigation of the JUCE documentation, particularly in selecting appropriate UI components and layout strategies during development.
