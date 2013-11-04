#include "Texture.h"

#include "BindLock.h"

#include <GL/glew.h>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

Texture::Texture() {
    glGenTextures(1, &_tex);
    BindLock<Texture> lock(*this);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

void Texture::setImage(BitmapPtr bitmap) {
    BindLock<Texture> lock(*this);
    unsigned pitch = FreeImage_GetPitch(bitmap.get());
    unsigned height = FreeImage_GetHeight(bitmap.get());
    unsigned bpp = FreeImage_GetBPP(bitmap.get());
    //assert(bpp == 8 || bpp == 32);
    unsigned format = bpp == 8 ? GL_RED : GL_RGBA;
    unsigned internal = bpp == 8 ? GL_R8 : GL_RGBA8;
    glTexImage2D(GL_TEXTURE_2D, 0, internal, pitch, height, 0, format, GL_UNSIGNED_BYTE, FreeImage_GetBits(bitmap.get()));
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, _tex);
    glActiveTexture(_tex);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(0);
}
