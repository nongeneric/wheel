#pragma once

#include <glm/glm.hpp>
#include <GL/glew.h>
#include <boost/chrono.hpp>

using fseconds = boost::chrono::duration<float>;

class Mesh;
class ScaleAnimation {
    fseconds _elapsed;
    fseconds _duration;
    GLfloat _from, _to;
    bool _isCompleted = true;
public:
    ScaleAnimation() = default;
    ScaleAnimation(fseconds duration, GLfloat from, GLfloat to);
    void animate(fseconds dt, std::function<Mesh&()> mesh);
    friend bool isCompleted(ScaleAnimation& a);
};
