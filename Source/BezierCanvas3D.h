#pragma once
#include <JuceHeader.h>

// ---------------------------------------------------------------------------
// Point3D — lightweight 3D point / vector used throughout this canvas
// ---------------------------------------------------------------------------
struct Point3D
{
    float x = 0.0f, y = 0.0f, z = 0.0f;

    Point3D operator+(const Point3D& o) const noexcept { return { x + o.x, y + o.y, z + o.z }; }
    Point3D operator-(const Point3D& o) const noexcept { return { x - o.x, y - o.y, z - o.z }; }
    Point3D operator*(float s)          const noexcept { return { x * s,   y * s,   z * s   }; }
    Point3D& operator+=(const Point3D& o) noexcept { x += o.x; y += o.y; z += o.z; return *this; }
};

inline float   dot3       (Point3D a, Point3D b) noexcept { return a.x*b.x + a.y*b.y + a.z*b.z; }
inline float   len3       (Point3D a)             noexcept { return std::sqrt(dot3(a, a)); }
inline Point3D normalize3 (Point3D a)             noexcept
{
    float l = len3(a);
    return (l > 1e-6f) ? a * (1.0f / l) : Point3D{ 0.0f, 0.0f, 0.0f };
}
inline Point3D cross3(Point3D a, Point3D b) noexcept
{
    return { a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x };
}
inline Point3D lerp3(Point3D a, Point3D b, double t) noexcept
{
    float f = static_cast<float>(t);
    return { a.x + f*(b.x - a.x), a.y + f*(b.y - a.y), a.z + f*(b.z - a.z) };
}

// ---------------------------------------------------------------------------
// BezierCanvas3D — interactive 3D Bézier curve canvas
// ---------------------------------------------------------------------------
class BezierCanvas3D : public juce::Component
{
public:
    enum class Method { NLI, BB, Midpoint };

    BezierCanvas3D();

    void setDegree(int d);
    void setMethod(Method m);
    void setT(double tt);
    void clearPoints();
    void resetCamera();
    void resetToDefaultIfEmpty();

    // Point access — used by the coord panel
    int     getPointCount()          const noexcept { return static_cast<int>(controlPoints.size()); }
    Point3D getPoint(int i)          const noexcept;
    void    setPoint(int i, Point3D p)     noexcept;  // sets value + repaints, no callback

    // Currently selected point index (-1 = none); persists after mouse release
    int getSelectedIndex() const noexcept { return selectedIndex; }

    // Fired when the selected point index changes (index == -1 means deselected)
    std::function<void(int index)>              onSelectionChanged;

    // Fired each time the selected point moves (drag or setPoint won't fire this)
    std::function<void(int index, Point3D pos)> onPointChanged;

    void paint           (juce::Graphics& g)                              override;
    void mouseDown       (const juce::MouseEvent& e)                      override;
    void mouseDrag       (const juce::MouseEvent& e)                      override;
    void mouseUp         (const juce::MouseEvent& e)                      override;
    void mouseWheelMove  (const juce::MouseEvent& e,
                          const juce::MouseWheelDetails& d)               override;

private:
    // ----- Bézier model -----
    int    degree = 3;
    Method method = Method::NLI;
    double t      = 0.5;

    std::vector<Point3D>             controlPoints;
    std::vector<std::vector<double>> choose;

    void buildChooseTable(int maxDegree);

    // ----- Camera -----
    float   camAzimuth   =  0.4f;
    float   camElevation =  0.35f;
    float   camDistance  = 10.0f;
    Point3D camPivot     = { 0.0f, 0.0f, 0.0f };

    Point3D getEye() const noexcept;
    void    getCameraAxes(Point3D& right, Point3D& up, Point3D& fwd) const noexcept;

    bool projectPoint(Point3D p,
                      Point3D eyePos, Point3D right, Point3D up, Point3D fwd,
                      float focalLen, float cx, float cy,
                      juce::Point<float>& out) const noexcept;

    bool projectPointFull(Point3D p, juce::Point<float>& out) const noexcept;

    // ----- Interaction state -----
    int               selectedIndex  = -1;   // persists across mouse up
    int               dragIndex      = -1;   // cleared on mouseUp
    bool              orbitDragging  = false;
    juce::Point<float> lastMousePos;

    static constexpr float hitRadius = 12.0f;

    int     pickPoint(juce::Point<float> screenPos) const noexcept;
    Point3D unprojectToGroundPlane(juce::Point<float> screenPos) const noexcept;

    void setSelectedIndex(int idx);  // updates selectedIndex + fires callback if changed

    // ----- Evaluation -----
    Point3D evalNLI3D(double tt,
                      std::vector<std::vector<Point3D>>* outTri = nullptr) const;
    Point3D evalBB3D (double tt) const;

    void subdivideMidpoint3D(const std::vector<Point3D>& pts, int depth,
                              std::vector<Point3D>& out) const;
    void buildPolyLine3D(std::vector<Point3D>& out) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(BezierCanvas3D)
};
