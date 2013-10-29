#include "OpenGLbasics.h"


Vertex::Vertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal)
    : pos(pos), uv(uv), normal(normal) { }

VAO::VAO() {
    glGenVertexArrays(1, &_vao);
}

void VAO::bind() {
    glBindVertexArray(_vao);
}

void VAO::unbind() {
    glBindVertexArray(0);
}


VertexBuffer createFullScreenVertices() {
    return VertexBuffer {{
            {glm::vec3 { -1, -1, 0 }, glm::vec2 { 0, 0 }},
            {glm::vec3 { 1, -1, 0 }, glm::vec2 { 1, 0 }},
            {glm::vec3 { -1, 1, 0.}, glm::vec2 { 0, 1 }},
            {glm::vec3 { 1, 1, 0 }, glm::vec2 { 1, 1 }}
        }};
}


IndexBuffer createFullScreenIndices() {
    return IndexBuffer{{
            0, 1, 3, 0, 2, 3
        }};
}
