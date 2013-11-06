#include "Painter2D.h"

#include <glm/gtc/matrix_transform.hpp>
#include <tuple>

Program createPainterProgram() {
    Program res;
    res.addVertexShader(
        SHADER_VERSION
        "in vec4 pos;\n" // layout(location = 0)
        "uniform mat4 transform;\n"
        "uniform vec4 color;\n"
        "out vec4 f_color;\n"
        "void main() {\n"
        "    gl_Position = transform * pos;\n"
        "    f_color = color;\n"
        "}"
    );
    res.addFragmentShader(
        SHADER_VERSION
        "in vec4 f_color;"
        "out vec4 outputColor;"
        "void main() {"
        "   outputColor = f_color;"
        "}"
    );
    res.bindAttribLocation(0, "pos");
    res.link();
    return res;
}


void Painter2D::initVao() {
    BindLock<VAO> lock(_vao);
    _vertices.bind();
    _indices.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    //glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
}

Painter2D::Painter2D()
    : _program{createPainterProgram()},
      _vertices{createFullScreenVertices()},
      _indices{createFullScreenIndices()}
{
    initVao();
    _uTransform = _program.getUniformLocation("transform");
    _uColor = _program.getUniformLocation("color");
}

void Painter2D::rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color) {
    _rects.emplace_back(pos * 2.0f, size, color);
}

void Painter2D::draw() {
    BindLock<Program> programLock(_program);
    BindLock<VAO> vaoLock(_vao);
    for (auto const& rect : _rects) {
        auto scale = glm::scale( {}, glm::vec3 { std::get<1>(rect).x, std::get<1>(rect).y, 1 });
        auto pos = std::get<0>(rect) + std::get<1>(rect) - 1.0f;
        auto translate = glm::translate( {}, glm::vec3 { pos.x, pos.y, 0 } );
        _program.setUniform(_uTransform, translate * scale);
        _program.setUniform(_uColor, std::get<2>(rect));
        glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, 0);
    }
}
