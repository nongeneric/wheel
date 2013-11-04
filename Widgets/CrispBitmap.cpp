#include "CrispBitmap.h"

#include "../Program.h"
#include "../OpenGLbasics.h"
#include "../Texture.h"

#include <glm/gtc/matrix_transform.hpp>

Program createBitmapProgram() {
    Program res;
    res.addVertexShader(
        SHADER_VERSION
        "in vec4 pos;" // layout(location = 0)
        "in vec2 uv;" // layout(location = 1)
        "uniform mat4 transform;"
        "out vec2 f_uv;"
        "void main() {"
        "    gl_Position = transform * pos;"
        "    f_uv = uv;"
        "}"
    );
    res.addFragmentShader(
        SHADER_VERSION
        "in vec2 f_uv;"
        "uniform sampler2D sampler;"
        "uniform vec3 color;"
        "out vec4 outputColor;"
        "void main() {"
        "   outputColor = vec4(color, texture2D(sampler, f_uv).r);"
        "}"
    );
    res.link();
    return res;
}

struct CrispBitmap::impl {
    Texture tex;
    VAO vao;
    VertexBuffer vertices;
    IndexBuffer indices;
    Program program;
    GLuint sampler;
    GLuint transformUniform;
    GLuint colorUniform;
    glm::mat4 transform;
    glm::mat4 externalTransform;
    long width = 0;
    long height = 0;
    glm::vec3 color;
    glm::vec2 framebuffer;
    void initVao() {
        BindLock<VAO> lock(vao);
        vertices.bind();
        indices.bind();
        tex.bind();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    }
    impl() : vertices(createFullScreenVertices()),
        indices(createFullScreenIndices()),
        program(createBitmapProgram())
    {
        sampler = program.getUniformLocation("sampler");
        transformUniform = program.getUniformLocation("transform");
        colorUniform = program.getUniformLocation("color");
        color = glm::vec3 { 1.0f, 1.0f, 1.0f };
        initVao();
    }
};

CrispBitmap::CrispBitmap()
    : m(new impl()) { }
CrispBitmap::CrispBitmap(CrispBitmap&& other)
    : m(std::move(other.m)) { }
CrispBitmap::~CrispBitmap()
{ }

void CrispBitmap::setBitmap(BitmapPtr bitmap) {
    assert(FreeImage_GetBPP(bitmap.get()) == 8);
    m->width = FreeImage_GetWidth(bitmap.get());
    m->height = FreeImage_GetHeight(bitmap.get());
    m->tex.setImage(bitmap);
}

void CrispBitmap::setColor(glm::vec3 color) {
    m->color = color;
}

void CrispBitmap::animate(fseconds) { }

void CrispBitmap::draw() {
    BindLock<Program> programLock(m->program);
    m->program.setUniform(m->sampler, 0);
    m->program.setUniform(m->transformUniform, m->externalTransform * m->transform);
    m->program.setUniform(m->colorUniform, m->color);
    BindLock<Texture> texLock(m->tex);
    BindLock<VAO> vaoLock(m->vao);
    glDrawElements(GL_TRIANGLES, m->indices.size(), GL_UNSIGNED_SHORT, 0);
}

void CrispBitmap::measure(glm::vec2, glm::vec2 framebuffer) {
    m->framebuffer = framebuffer;
}

void CrispBitmap::arrange(glm::vec2 pos, glm::vec2) {
    float normWidth = (float)m->width / m->framebuffer.x;
    float normHeight = (float)m->height / m->framebuffer.y;
    auto scale = glm::scale( {}, glm::vec3 {normWidth, normHeight, 1});
    float normDx = pos.x / m->framebuffer.x * 2;
    float normDy = pos.y / m->framebuffer.y * 2;
    auto translate = glm::translate( {}, glm::vec3 {normDx + normWidth - 1, normDy + normHeight - 1, 0});
    m->transform = translate * scale;
}

glm::vec2 CrispBitmap::desired() {
    return glm::vec2 { m->width, m->height };
}

void CrispBitmap::setTransform(glm::mat4 transform) {
    m->externalTransform = transform;
    m->externalTransform[3][0] /= m->framebuffer.x / 2;
    m->externalTransform[3][1] /= m->framebuffer.y / 2;
}
