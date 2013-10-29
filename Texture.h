#pragma once

class Texture {
    unsigned _tex;
public:
    Texture();
    void setImage(void* buffer, unsigned width, unsigned height);
    void bind();
    void unbind();
};
