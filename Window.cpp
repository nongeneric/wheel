#include "Window.h"

#include "vformat.h"

#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>

std::vector<Monitor> getMonitors() {
    std::vector<Monitor> res;
    auto monitorsCount = SDL_GetNumVideoDisplays();

    for (auto monitor = 0; monitor < monitorsCount; ++monitor) {
        auto modesCount = SDL_GetNumDisplayModes(monitor);
        std::vector<MonitorMode> vmodes;
        for (auto mode = 0; mode < modesCount; ++mode) {
            SDL_DisplayMode displayMode{};
            SDL_GetDisplayMode(monitor, mode, &displayMode);
            vmodes.push_back({displayMode.w, displayMode.h, displayMode.refresh_rate});
        }
        auto name = boost::lexical_cast<std::string>(monitor);

        SDL_Rect rect;
        SDL_GetDisplayBounds(monitor, &rect);

        res.push_back({monitor, name, rect.w, rect.h, vmodes});
    }
    return res;
}

Monitor findMonitor(std::string name) {
    auto index = boost::lexical_cast<unsigned>(name);
    auto monitors = getMonitors();
    if (index >= monitors.size())
        return monitors[0];
    return monitors[index];
}

void glDebugCallbackFunction(
    GLenum source, GLenum, GLuint, GLenum severity, GLsizei, const GLchar* message, const void*) {
    auto sourceStr = source == GL_DEBUG_SOURCE_API ? "Api"
                   : source == GL_DEBUG_SOURCE_WINDOW_SYSTEM ? "WindowSystem"
                   : source == GL_DEBUG_SOURCE_SHADER_COMPILER ? "ShaderCompiler"
                   : source == GL_DEBUG_SOURCE_THIRD_PARTY ? "ThirdParty"
                   : source == GL_DEBUG_SOURCE_APPLICATION ? "Application"
                   : "Other";
    std::cout << "gl callback [source" << sourceStr << "]: " << message << std::endl;
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        exit(1);
}

Window::Window(std::string title, bool fullscreen, unsigned width, unsigned height, std::string monitor) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER | SDL_INIT_HAPTIC))
        throw std::runtime_error(vformat("sdl2 init failed (%s)", SDL_GetError()));

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    auto xPos = SDL_WINDOWPOS_CENTERED, yPos = SDL_WINDOWPOS_CENTERED;

    auto flags = SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE;
    if (fullscreen) {
        flags |= SDL_WINDOW_BORDERLESS;
        auto monitorInfo = findMonitor(monitor);
        xPos = SDL_WINDOWPOS_CENTERED_DISPLAY(monitorInfo.id);
        yPos = SDL_WINDOWPOS_CENTERED_DISPLAY(monitorInfo.id);
        width = monitorInfo.currentWidth;
        height = monitorInfo.currentHeight;
    }

    _window = SDL_CreateWindow(title.c_str(), xPos, yPos, width, height, flags);

    if (_window == nullptr)
        throw std::runtime_error("window creation failed");

    if (!SDL_GL_CreateContext(_window))
        throw std::runtime_error(vformat("sdl2 failed to create OpenGL context (%s)", SDL_GetError()));

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glew init failure");

#ifdef DEBUG
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);
#endif

    std::cout << (char*)glGetString(GL_VERSION) << std::endl;
}

Window::~Window() {
    SDL_DestroyWindow(_window);
}

void Window::swap() {
    SDL_GL_SwapWindow(_window);
}

bool Window::shouldClose() {
    SDL_Event event;
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT)
            return true;
    }
    return false;
}

glm::vec2 Window::getFramebufferSize() {
    int width, height;
    SDL_GetWindowSize(_window, &width, &height);
    return glm::vec2(width, height);
}

void* Window::handle() const {
    return nullptr;
}
