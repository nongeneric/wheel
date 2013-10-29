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

void Texture::setImage(void *buffer, unsigned width, unsigned height) {
    BindLock<Texture> lock(*this);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
}

void Texture::bind() {
    glBindTexture(GL_TEXTURE_2D, _tex);
    glActiveTexture(_tex);
}

void Texture::unbind() {
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(0);
}
