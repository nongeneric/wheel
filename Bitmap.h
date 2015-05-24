#pragma once

#include <vector>
#include <memory>

class Bitmap {
    std::shared_ptr<std::vector<char>> _vec;
    unsigned _width;
    unsigned _height;
    unsigned _bpp;
    unsigned _pitch;
    unsigned pixelPos(unsigned x, unsigned y) const;
public:
    Bitmap();
    Bitmap(unsigned width, unsigned height, unsigned bpp);
    Bitmap(void* data, unsigned width, unsigned height, unsigned pitch, unsigned bpp, bool topLeftFirst);
    Bitmap copy(unsigned left, unsigned top, unsigned right, unsigned bottom) const;
    char* data();
    unsigned width() const;
    unsigned height() const;
    unsigned bpp() const;
    unsigned pitch() const;
    unsigned pixel(unsigned x, unsigned y) const;
    void setPixel(unsigned x, unsigned y, unsigned value);
    void fill(unsigned value);
    void blendPaste(Bitmap other, unsigned left, unsigned bottom);
    virtual ~Bitmap();
};
