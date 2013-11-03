#include "SpreadAnimator.h"
#include "../rstd.h"
#include <glm/gtc/matrix_transform.hpp>

float SpreadAnimator::animCurve(float a, float b, float ratio) {
    return a + ratio * ratio * ratio * ratio * (b - a);
}

SpreadAnimator::SpreadAnimator(std::vector<ISpreadAnimationLine *> widgets) : _widgets(widgets) { }

float SpreadAnimator::calcWidthAfterMeasure(float available) {
    float width = 0;
    for (auto const& w : _widgets) {
        width = std::max(width, w->width());
    }
    _positions.clear();
    for (auto& w : _widgets) {
        _positions.push_back({(width - available) / 2, (width - w->width()) / 2, (available - w->width()) / 2});
    }
    return width;
}

std::vector<float> SpreadAnimator::getCenters() {
    return rstd::fmap<Pos, float>(_positions, [](Pos const& p) {
        return p.center;
    });
}

void SpreadAnimator::beginAnimating(bool assemble) {
    _animating = true;
    _elapsed = fseconds();
    _isAssembling = assemble;
}

void SpreadAnimator::animate(fseconds dt) {
    if (_positions.empty())
        return;
    if (!_animating)
        return;
    fseconds duration(0.4f);
    if (_elapsed > duration) {
        _animating = false;
        return;
    }
    _elapsed += dt;
    float ratio = std::min(_elapsed.count() / duration.count(), 1.0f);
    ratio = _isAssembling ? 1.0f - ratio : ratio;
    for (unsigned i = 0; i < _widgets.size(); ++i) {
        if (i & 1) {
            glm::vec3 trans {
                animCurve(.0f, _positions.at(i).right - _positions.at(i).center, ratio),
                        .0f,
                        .0f
            };
            _widgets[i]->setTransform(glm::translate( {}, trans));
        } else {
            glm::vec3 trans {
                animCurve(.0f, _positions.at(i).left - _positions.at(i).center, ratio),
                        .0f,
                        .0f
            };
            _widgets[i]->setTransform(glm::translate( {}, trans));
        }
    }
}
