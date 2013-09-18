#pragma once

#include <GL/glew.h>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

#include <vector>
#include <string>

class Program {
    std::vector<GLuint> _shaders;
    GLuint _program;

    GLuint createShader(const std::string& text, GLenum shaderType);
public:
    void addVertexShader(const std::string& text);
    void addFragmentShader(const std::string& text);
    void link();
    GLuint getUniformLocation(std::string const& name);
    void setUniform(GLuint location, glm::mat4 const& matrix);
    void setUniform(GLuint location, glm::vec3 const& matrix);
    void setUniform(GLuint location, GLint value);
    void setUniform(GLuint location, GLfloat value);
    void bind();
    void unbind();
};