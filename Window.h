#pragma once

#include "Config.h"

#include <GL/glew.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>
#include <string>
#include <vector>

#include <glm/glm.hpp>

class Window {
    SDL_Window* _window;
public:
    Window(std::string title, DisplayMode displayMode, unsigned width, unsigned height, unsigned monitor);
    ~Window();
    void swap();
    bool shouldClose();
    glm::vec2 getFramebufferSize();
    void* handle() const;
};

struct Monitor {
    unsigned id;
    std::string name;
    int currentWidth;
    int currentHeight;
};

std::vector<Monitor> getMonitors();
