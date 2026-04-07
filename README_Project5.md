# Project 5 – Interactive B-Spline Visualization (De Boor Algorithm)

Rahul Nair (rahul.nair@digipen.edu)

## Overview

This project is an interactive C++ application built using the JUCE framework to visualize a weighted sum of uniform B-spline basis functions evaluated via the De Boor algorithm.

Users can drag coefficient control points up and down to reshape the curve in real time. The underlying knot sequence is uniform (`t_k = k`), and the curve is plotted over the full interval `[0, N]`, tapering naturally to zero at the boundaries.

## Running the Project

Double-click the `.exe` file to launch the application.

## Building the Project

Using the `Source` folder provided, generate a project using the JUCE Projucer and build from any compatible IDE.

## Architecture

The application follows the same modular, component-based structure as the previous projects:

- A UI/controller component manages user-facing controls:
  - Degree slider (`d`, range 1–20)
  - N slider (range `[2d+2, 42]`, adjusts the number of knot intervals)
  - Reset button (returns all coefficients to 1.0)

- A single rendering and interaction component encapsulates:
  - Coefficient dot placement and mouse drag interaction
  - De Boor algorithm evaluation
  - Curve and axis drawing

## Mathematics

The curve is defined as a weighted sum of B-spline basis functions:

```
f(t) = c_0 * B^d_0(t) + c_1 * B^d_1(t) + ... + c_{N-d-1} * B^d_{N-d-1}(t)
```

over the uniform knot sequence `t_k = k`, for `t` in `[0, N]`.

There are `N - d` basis functions (and thus `N - d` draggable coefficients). The coefficient dots are placed at x-positions:

```
x_i = (d+1)/2 + i,   i = 0, 1, ..., N-d-1
```

which are integer or half-integer values depending on the parity of `d`.

### De Boor Algorithm

For a given `t`, let `J = floor(t)` (clamped to `[0, N-1]`). Starting from `c^[0]_i = c_i`, the algorithm performs `d` stages of nested linear interpolation:

```
c^[p]_i = ((t - i) / (d - p + 1)) * c^[p-1]_i
        + ((i + d - p + 1 - t) / (d - p + 1)) * c^[p-1]_{i-1}
```

for `i = J, J-1, ..., J-d+p` at each stage `p = 1, ..., d`. The result is `f(t) = c^[d]_J`.

Coefficients outside the range `[0, N-d-1]` are treated as zero, which causes the curve to taper smoothly to zero on both sides of the active interval `[d, N-d]`.

## Features

- Weighted sum of uniform B-spline basis functions of arbitrary degree
- Real-time dragging of coefficient control points with immediate curve update
- De Boor algorithm (Nested Linear Interpolation) for evaluation
- Curve plotted over the full knot range `[0, N]` with natural tapering to zero
- Adjustable degree `d` (1–20) and knot count `N` (up to 42)
- Default `N = d + 10` on degree change
- N slider automatically disabled when only one valid value exists (e.g. `d = 20`, `N = 42`)
- Reset button to restore all coefficients to 1.0 (partition-of-unity state)

## AI Assistance Disclosure

AI assistance was used to help navigate JUCE documentation for UI component layout and to cross-check the De Boor recurrence formula against the project specification during implementation.
