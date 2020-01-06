#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

class Window {
    GLFWwindow* _window;
public:
    Window(std::string title, bool fullscreen, unsigned width, unsigned height, std::string monitor);
    void swap();
    bool shouldClose();
    glm::vec2 getCursorPos();
    glm::vec2 getFramebufferSize();
    GLFWwindow* handle() const;
};

struct MonitorMode {
    int width;
    int height;
    int refreshRate;
};

struct Monitor {
    std::string name;
    std::vector<MonitorMode> modes;
};

std::vector<Monitor> getMonitors();
