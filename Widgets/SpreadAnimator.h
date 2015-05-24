#pragma once

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

#include <boost/chrono.hpp>
#include <vector>

using fseconds = boost::chrono::duration<float>;

class ISpreadAnimationLine {
public:
    virtual float width() = 0;
    virtual void setTransform(glm::mat4) = 0;
};

class SpreadAnimator {
    struct Pos {
        float left;
        float center;
        float right;
    };
    fseconds _elapsed;
    std::vector<ISpreadAnimationLine*> _widgets;
    std::vector<Pos> _positions;
    bool _isAssembling;
    bool _animating = false;
    float animCurve(float a, float b, float ratio);
public:
    SpreadAnimator(std::vector<ISpreadAnimationLine*> widgets);
    float calcWidthAfterMeasure(float availableWidth);
    std::vector<float> getCenters();
    void beginAnimating(bool assemble);
    void animate(fseconds dt);
};
