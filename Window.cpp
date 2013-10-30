#include "Window.h"

#include <stdexcept>
#include <iostream>

Window::Window(std::string title, bool fullscreen, unsigned width, unsigned height) {
    if (!glfwInit())
        throw std::runtime_error("glfw init failure");
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    //glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    _window = glfwCreateWindow(width, height, title.c_str(), fullscreen ? glfwGetPrimaryMonitor() : NULL, NULL);
    if (_window == nullptr)
        throw std::runtime_error("window init failure");
    glfwMakeContextCurrent(_window);
    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glew init failure");
    std::cout << (char*)glGetString(GL_VERSION) << std::endl;
}


void Window::swap() {
    glfwSwapBuffers(_window);
    glfwPollEvents();
}


bool Window::shouldClose() {
    return glfwWindowShouldClose(_window);
}


int Window::getKey(int key) {
    return glfwGetKey(_window, key);
}


int Window::getMouseButton(int button) {
    return glfwGetMouseButton(_window, button);
}


glm::vec2 Window::getCursorPos() {
    double x, y;
    glfwGetCursorPos(_window, &x, &y);
    return glm::vec2(x, y);
}


glm::vec2 Window::getFramebufferSize() {
    int width, height;
    glfwGetFramebufferSize(_window, &width, &height);
    return glm::vec2(width, height);
}
