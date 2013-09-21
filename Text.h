#pragma once

#include <FreeImage.h>
#include <memory>
#include <string>

typedef std::shared_ptr<FIBITMAP> BitmapPtr;

class Text {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Text();
    BitmapPtr renderText(std::string str, unsigned pxHeight);
    ~Text();
};
