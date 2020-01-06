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

void Camera::reset() {
    _angles = _defaultAngles;
    _pos = _defaultPos;
}

CameraController::CameraController(Window* window, Camera* camera)
    : _camera(camera), _window(window) {}

void CameraController::advance() {
    _leftPressed = false;
    _rightPressed = false;
    _upPressed = false;
    _downPressed = false;
}
