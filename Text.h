#pragma once

#include "Bitmap.h"
#include <memory>
#include <string>

class Text {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Text();
    Bitmap renderText(std::string str, unsigned pxHeight);
    ~Text();
};
