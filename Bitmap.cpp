#include "Bitmap.h"

#include <assert.h>
#include <string.h>
#include <algorithm>

unsigned Bitmap::pixelPos(unsigned x, unsigned y) const {
    assert(x < _width);
    assert(y < _height);
    auto pos = y * _pitch + x * _bpp / 8;
    assert(pos + _bpp / 8 <= _vec->size());
    return pos;
}

Bitmap::Bitmap() { }

Bitmap::Bitmap(unsigned width, unsigned height, unsigned bpp)
    : _width(width), _height(height), _bpp(bpp), _pitch(width * bpp / 8)
{
    _vec.reset(new std::vector<char>(width * height * bpp / 8));
}

Bitmap::Bitmap(void *data, unsigned width, unsigned height, unsigned pitch, unsigned bpp, bool topLeftFirst)
    : _width(width), _height(height), _bpp(bpp), _pitch(pitch)
{
    _vec.reset(new std::vector<char>(height * pitch));
    if (topLeftFirst) {
        auto pos = &(*_vec)[0];
        auto sourcePos = (char*)data + (height - 1) * pitch;
        for (auto row = 0u; row < height; ++row) {
            std::copy(sourcePos, sourcePos + pitch, pos);
            pos += pitch;
            sourcePos -= pitch;
        }
    } else {
        std::copy((char*)data, (char*)data + _vec->size(), begin(*_vec));
    }
}

Bitmap Bitmap::copy(unsigned left, unsigned top, unsigned right, unsigned bottom) const {
    Bitmap clone(right - left + 1, top - bottom + 1, _bpp);
    for (auto y = bottom; y <= top; ++y) {
        for (auto x = left; x <= right; ++x) {
            clone.setPixel(x - left, y - bottom, pixel(x, y));
        }
    }
    return clone;
}

char *Bitmap::data() {
    return &(*_vec)[0];
}

unsigned Bitmap::width() const {
    return _width;
}

unsigned Bitmap::height() const {
    return _height;
}

unsigned Bitmap::bpp() const {
    return _bpp;
}

unsigned Bitmap::pitch() const {
    return _pitch;
}

unsigned Bitmap::pixel(unsigned x, unsigned y) const {
    unsigned val = 0;
    memcpy(&val, _vec->data() + pixelPos(x, y), _bpp / 8);
    auto res = val & ~(-1u << _bpp);
    auto arr = (char*)&res;
    std::reverse(arr, arr + _bpp / 8);
    return res;
}

void Bitmap::setPixel(unsigned x, unsigned y, unsigned value) {
    char* arr = reinterpret_cast<char*>(&value);
    auto bytes = &_vec->at(pixelPos(x, y));
    std::copy(arr, arr + _bpp / 8, bytes);
    std::reverse(bytes, bytes + _bpp / 8);
}

void Bitmap::fill(unsigned value) {
    assert(_bpp == 8);
    std::fill(begin(*_vec), end(*_vec), (char)value);
}

void Bitmap::blendPaste(Bitmap other, unsigned left, unsigned bottom) {
    assert(_bpp == 8 && other.bpp() == 8);
    assert(left + other.width() <= width() &&
           bottom + other.height() <= height());
    for (auto row = 0u; row < other.height(); ++row) {
        for (auto col = 0u; col < other.width(); ++col) {
            setPixel(col + left, row + bottom, other.pixel(col, row)
                     + pixel(col + left, row + bottom));
        }
    }
}

Bitmap::~Bitmap() { }
