#include "Program.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <functional>

std::string getGlInfoLog(
        std::function<void(GLuint, GLenum, GLint*)> param,
        std::function<void(GLuint, GLsizei, GLsizei*, GLchar*)> getter,
        GLint object)
{
    GLint len;
    param(object, GL_INFO_LOG_LENGTH, &len);
    std::string info(len, 0);
    getter(object, info.size(), NULL, &info[0]);
    return info;
}

GLuint Program::createShader(const std::string &text, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    const char* ptr = text.c_str();
    int len = text.size();
    glShaderSource(shader, 1, &ptr, &len);
    glCompileShader(shader);

    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (status == GL_FALSE) {
        std::cout << getGlInfoLog(glGetShaderiv, glGetShaderInfoLog, shader);
        throw std::runtime_error("shader compilation failure");
    }
    return shader;
}

void Program::addVertexShader(const std::string &text) {
    _shaders.push_back(createShader(text, GL_VERTEX_SHADER));
}

void Program::addFragmentShader(const std::string &text) {
    _shaders.push_back(createShader(text, GL_FRAGMENT_SHADER));
}

void Program::link() {
    _program = glCreateProgram();
    for (GLuint shader : _shaders) {
        glAttachShader(_program, shader);
    }
    glLinkProgram(_program);
    GLint status;
    glGetProgramiv(_program, GL_LINK_STATUS, &status);
    if (status == GL_FALSE) {
        std::cout << getGlInfoLog(glGetProgramiv, glGetProgramInfoLog, _program);
        throw std::runtime_error("program compilation failure");
    }
    for (GLuint shader : _shaders) {
        glDetachShader(_program, shader);
        glDeleteShader(shader);
    }
}

GLuint Program::getUniformLocation(const std::string &name) {
    GLuint loc = glGetUniformLocation(_program, name.c_str());
    if (loc == (GLuint)-1) {
        throw std::runtime_error("uniform get location error");
    }
    return loc;
}

void Program::setUniform(GLuint location, const glm::mat4 &matrix) {
    glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
    GLenum error = glGetError();
    if (error != GL_NO_ERROR && error != GL_INVALID_ENUM ) {
        throw std::runtime_error("uniform set error");
    }
}

void Program::setUniform(GLuint location, const glm::vec3 &matrix) {
    glUniform3fv(location, 3, glm::value_ptr(matrix));
}

void Program::setUniform(GLuint location, GLint value) {
    glUniform1i(location, value);
}

void Program::setUniform(GLuint location, GLfloat value) {
    glUniform1f(location, value);
}

void Program::bind() {
    glUseProgram(_program);
}

void Program::unbind() {
    glUseProgram(0);
}
