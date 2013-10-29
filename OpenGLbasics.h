#pragma once

#define GLM_FORCE_CXX11

#include "BindLock.h"
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <vector>

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    Vertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal = glm::vec3 {});
};

template <typename BaseType, int Type>
class Buffer {
    GLuint _buffer;
    unsigned _size;
public:
    Buffer(std::vector<BaseType> const& vec) {
        _size = vec.size();
        glGenBuffers(1, &_buffer);
        BindLock<Buffer<BaseType, Type>> _(*this);
        auto bytes = vec.size() * sizeof(BaseType);
        glBufferData(Type, bytes, vec.data(), GL_STATIC_DRAW);
    }
    void bind() {
        glBindBuffer(Type, _buffer);
    }
    void unbind() {
        glBindBuffer(Type, 0);
    }
    unsigned size() {
        return _size;
    }
};

class VAO {
    GLuint _vao;
public:
    VAO();
    void bind();
    void unbind();
};

typedef Buffer<Vertex, GL_ARRAY_BUFFER> VertexBuffer;
typedef Buffer<GLshort, GL_ELEMENT_ARRAY_BUFFER> IndexBuffer;

VertexBuffer createFullScreenVertices();
IndexBuffer createFullScreenIndices();
