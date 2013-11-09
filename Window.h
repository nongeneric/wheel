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
    int getKey(int key);
    int getMouseButton(int button);
    glm::vec2 getCursorPos();
    glm::vec2 getFramebufferSize();
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

class MonitorManager {
public:

};

std::vector<Monitor> getMonitors();
