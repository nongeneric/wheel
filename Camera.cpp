#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>
#include "Keyboard.h"

glm::mat4 Camera::view() {
    glm::mat4 rotation = glm::rotate( glm::mat4(), -_angles.x, glm::vec3 { 1, 0, 0 } ) *
            glm::rotate( glm::mat4(), -_angles.y, glm::vec3 { 0, 1, 0 } ) *
            glm::rotate( glm::mat4(), -_angles.z, glm::vec3 { 0, 0, 1 } );
    glm::mat4 translation = glm::translate( {}, -_pos );
    return translation * rotation;
}

void Camera::updateKeyboard(bool left, bool right, bool up, bool down) {
    _pos += glm::vec3 {
            -0.25 * left + 0.25 * right,
            0,
            -0.25 * up + 0.25 * down
    };
}

void Camera::updateMouse(Window &window) {
    bool mouseLeftPressed = window.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    if (mouseLeftPressed) {
        glm::vec2 delta = window.getCursorPos() - _cursor;
        _angles += glm::vec3 { -0.5 * delta.y, -0.5 * delta.x, 0 };
    }
    _cursor = window.getCursorPos();
}

void Camera::reset() {
    _angles = _defaultAngles;
    _pos = _defaultPos;
}


CameraController::CameraController(Window *window, Camera *camera, Keyboard *keys)
    : _camera(camera), _window(window)
{
    keys->onRepeat(GLFW_KEY_KP_4, fseconds(0.015f), State::Game, [&]() {
        _leftPressed = true;
    });
    keys->onRepeat(GLFW_KEY_KP_6, fseconds(0.015f), State::Game, [&]() {
        _rightPressed = true;
    });
    keys->onRepeat(GLFW_KEY_KP_8, fseconds(0.015f), State::Game, [&]() {
        _upPressed = true;
    });
    keys->onRepeat(GLFW_KEY_KP_2, fseconds(0.015f), State::Game, [&]() {
        _downPressed = true;
    });
    keys->onDown(GLFW_KEY_KP_5, State::Game, [&]() {
        _camera->reset();
    });
    keys->enableHandler(State::Game);
}

void CameraController::advance() {
    _leftPressed = false;
    _rightPressed = false;
    _upPressed = false;
    _downPressed = false;
    _camera->updateKeyboard(_leftPressed, _rightPressed, _upPressed, _downPressed);
    _camera->updateMouse(*_window);
}
