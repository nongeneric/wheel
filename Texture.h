#pragma once

#include "Text.h"

class Texture {    
    unsigned _tex;
public:
    Texture();
    void setImage(Bitmap bitmap);
    void bind();
    void unbind();
};
