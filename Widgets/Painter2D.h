#pragma once

#include "../Program.h"
#include "../OpenGLbasics.h"

class Painter2D {
    Program _program;
    VAO _vao;
    VertexBuffer _vertices;
    IndexBuffer _indices;
    std::vector<std::tuple<glm::vec2, glm::vec2, glm::vec4>> _rects;
    GLuint _uTransform;
    GLuint _uColor;
    void initVao();
public:
    Painter2D();
    void rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color);
    void draw();
};
