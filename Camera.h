#pragma once

#include "Window.h"

class Camera {
    glm::vec3 _defaultAngles{0, 0, 0};
    glm::vec3 _defaultPos{0, 13, 50};
    glm::vec3 _angles = _defaultAngles;
    glm::vec3 _pos = _defaultPos;
    glm::vec2 _cursor{ 300, 300 };
public:
    glm::mat4 view();
    void reset();
};

class Keyboard;
class CameraController {
    Camera* _camera;
    Window* _window;
    bool _leftPressed;
    bool _rightPressed;
    bool _upPressed;
    bool _downPressed;
public:
    CameraController(Window* window, Camera *camera);
    void advance();
};
