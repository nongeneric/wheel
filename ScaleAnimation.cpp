#include "ScaleAnimation.h"
#include "Mesh.h"

ScaleAnimation::ScaleAnimation(fseconds duration, GLfloat from, GLfloat to)
    : _duration(duration), _from(from), _to(to), _isCompleted(false) { }

void ScaleAnimation::animate(fseconds dt, std::function<Mesh &()> mesh) {
    if (_isCompleted)
        return;
    _elapsed += dt;
    GLfloat factor;
    if (_elapsed > _duration) {
        _isCompleted = true;
        factor = _to;
    } else {
        factor = _from + (_to - _from) * (_elapsed / _duration);
    }
    setScale(mesh(), glm::vec3 { factor, factor, factor });
}
