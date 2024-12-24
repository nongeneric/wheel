#pragma once

#include "Bitmap.h"

#include <glm/glm.hpp>

#include <memory>
#include <string>

class Text {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Text();
    Bitmap renderText(std::string str, glm::vec2 framebuffer, unsigned pxHeight);
    ~Text();
};
