#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

class Window {
    GLFWwindow* _window;
public:
    Window(std::string title);
    void swap();
    bool shouldClose();
    int getKey(int key);
    int getMouseButton(int button);
    glm::vec2 getCursorPos();
    glm::vec2 getFramebufferSize();
};
