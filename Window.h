#pragma once

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <string>
#include <vector>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

class Window {
    SDL_Window* _window;
public:
    Window(std::string title, bool fullscreen, unsigned width, unsigned height, std::string monitor);
    ~Window();
    void swap();
    bool shouldClose();
    glm::vec2 getFramebufferSize();
    void* handle() const;
};

struct MonitorMode {
    int width;
    int height;
    int refreshRate;
};

struct Monitor {
    int id;
    std::string name;
    int currentWidth;
    int currentHeight;
    std::vector<MonitorMode> modes;
};

std::vector<Monitor> getMonitors();
