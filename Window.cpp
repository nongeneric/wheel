#include "Window.h"

#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>

std::vector<Monitor> getMonitors() {
    std::vector<Monitor> res;
    int monitorsCount;
    GLFWmonitor** monitors = glfwGetMonitors(&monitorsCount);
    for (int i = 0; i < monitorsCount; ++i) {
        int modesCount;
        const GLFWvidmode* modes = glfwGetVideoModes(monitors[i], &modesCount);
        std::vector<MonitorMode> vmodes;
        for (int j = 0; j < modesCount; ++j) {
            vmodes.push_back({modes[j].width, modes[j].height, modes[j].refreshRate});
        }
        auto name = boost::lexical_cast<std::string>(i);
        res.push_back({name, vmodes});
    }
    return res;
}

GLFWmonitor* findMonitor(std::string name) {
    int count;
    int index = boost::lexical_cast<int>(name);
    GLFWmonitor** monitors = glfwGetMonitors(&count);
    if (count > index)
        return monitors[index];
    return glfwGetPrimaryMonitor();
}

void glDebugCallbackFunction(GLenum source,
            GLenum ,
            GLuint ,
            GLenum severity,
            GLsizei ,
            const GLchar *message,
            const void *) {
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
    if (!glfwInit())
        throw std::runtime_error("glfw init failure");
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    _window = glfwCreateWindow(width, height, title.c_str(), fullscreen ? findMonitor(monitor) : NULL, NULL);
    if (_window == nullptr)
        throw std::runtime_error("window init failure");
    glfwMakeContextCurrent(_window);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
        throw std::runtime_error("glew init failure");

#ifndef NDEBUG
    glEnable(GL_DEBUG_OUTPUT);
#endif
    glDebugMessageCallback(&glDebugCallbackFunction, nullptr);

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
