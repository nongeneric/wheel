#pragma once

#include <FreeImage.h>
#include <memory>
#include <string>

typedef std::shared_ptr<FIBITMAP> BitmapPtr;
std::shared_ptr<FIBITMAP> make_bitmap_ptr(FIBITMAP* raw);

class Text {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Text();
    BitmapPtr renderText(std::string str, unsigned pxHeight);
    ~Text();
};
