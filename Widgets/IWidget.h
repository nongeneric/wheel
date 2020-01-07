#pragma once

#include <chrono>

using fseconds = std::chrono::duration<float>;

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

struct IWidget {
    virtual void animate(fseconds dt) = 0;
    virtual void draw() = 0;
    virtual void measure(glm::vec2 available, glm::vec2 framebuffer) = 0;
    virtual void arrange(glm::vec2 pos, glm::vec2 size) = 0;
    virtual glm::vec2 desired() = 0;
    virtual void setTransform(glm::mat4 transform) = 0;
};
