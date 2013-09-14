#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <assert.h>

class Window {
    GLFWwindow* _window;
public:
    Window(std::string title) {
        if (!glfwInit())
            throw std::runtime_error("glfw init failure");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        _window = glfwCreateWindow(640, 480, title.c_str(), NULL, NULL);
        if (_window == nullptr)
            throw std::runtime_error("window init failure");
        glfwMakeContextCurrent(_window);
        glewExperimental = GL_TRUE;
        if (glewInit() != GLEW_OK)
            throw std::runtime_error("glew init failure");
        BOOST_LOG_TRIVIAL(info) << glGetString(GL_VERSION);
    }
    void swap() {
        glfwSwapBuffers(_window);
        glfwPollEvents();
    }
    bool shouldClose() {
        return glfwWindowShouldClose(_window);
    }
    // uses the right program for each mesh/primitive
    // void draw(many meshes)
    //   group by program
    //   for each program
    //     BindLock pl(program)
    //     ... set uniforms, how? (mesh or program or..)
    //     for each mesh
    //       mesh.draw();
    void setCamera(int matrix);
};

template<typename Bindable>
class BindLock {
    Bindable& _bindable;
    BindLock(const BindLock&) = delete;
    BindLock& operator=(const BindLock&) = delete;
public:
    BindLock(Bindable& bindable) : _bindable(bindable) {
        _bindable.bind();
    }
    ~BindLock() {
        _bindable.unbind();
    }
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

class Program {
    std::vector<GLuint> _shaders;
    GLuint _program;
    GLuint createShader(const std::string& text, GLenum shaderType) {
        GLuint shader = glCreateShader(shaderType);
        const char* ptr = text.c_str();
        int len = text.size();
        glShaderSource(shader, 1, &ptr, &len);
        glCompileShader(shader);

        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
        if (status == GL_FALSE) {
            BOOST_LOG_TRIVIAL(error) << getGlInfoLog(glGetShaderiv, glGetShaderInfoLog, shader);
            throw std::runtime_error("shader compilation failure");
        }
        return shader;
    }

public:
    void addVertexShader(const std::string& text) {
        _shaders.push_back(createShader(text, GL_VERTEX_SHADER));
    }
    void addFragmentShader(const std::string& text) {
        _shaders.push_back(createShader(text, GL_FRAGMENT_SHADER));
    }
    void compile() {
        _program = glCreateProgram();
        for (GLuint shader : _shaders) {
            glAttachShader(_program, shader);
        }
        glLinkProgram(_program);
        GLint status;
        glGetProgramiv(_program, GL_LINK_STATUS, &status);
        if (status == GL_FALSE) {
            BOOST_LOG_TRIVIAL(error) << getGlInfoLog(glGetProgramiv, glGetProgramInfoLog, _program);
            throw std::runtime_error("program compilation failure");
        }
        for (GLuint shader : _shaders) {
            glDetachShader(_program, shader);
            glDeleteShader(shader);
        }
    }
    void setUniform(int name, int value) { }
    void bind() {
        glUseProgram(_program);
    }
    void unbind() {
        glUseProgram(0);
    }
};

class VAO {
    GLuint _vao;
public:
    VAO() {
        glGenVertexArrays(1, &_vao);
    }
    void bind() {
        glBindVertexArray(_vao);
    }
    void unbind() {
        glBindVertexArray(0);
    }
};

typedef Buffer<glm::vec4, GL_ARRAY_BUFFER> VertexBuffer;
typedef Buffer<GLshort, GL_ELEMENT_ARRAY_BUFFER> IndexBuffer;

void init_vao(VertexBuffer buffer) {
    buffer.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    auto colorsOffset = reinterpret_cast<void*>(buffer.size() * sizeof(glm::vec4) / 2);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colorsOffset);
}

// contains one vao
class Mesh {
    VAO _vao;
    unsigned _indicesCount;
public:
    Mesh(VertexBuffer vertices, IndexBuffer indices) {
        _indicesCount = indices.size();
        BindLock<VAO> vaoLock(_vao);
        init_vao(vertices);
        indices.bind();
    }
    // extract from here, bind a single program once,
    // draw everything with it, move to another program etc.
    void draw(Program program, int cameraProjMatrix = 0) {
        BindLock<Program> programLock(program);
        // set pipelineMatrix uniform = cameraProjMatrix * meshPos
        BindLock<VAO> vaoLock(_vao);
        glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_SHORT, 0);
    }
};

class View {
public:
    void setPosition(glm::vec4 cartesian);
    void setOrientation(glm::vec2 sphericalTarget, float angle) {

    }
    glm::mat4 getMatrix();
};

std::string vertexShader =
        "#version 330\n"

        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec4 color;\n"
        "void main() {\n"
        "    gl_Position = position;\n"
        "}\n"
        ;

std::string fragmentShader =
        "#version 330\n"

        "out vec4 outputColor;\n"
        "void main() {\n"
        "   outputColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);\n"
        "}"
        ;

int main() {
    Window window("hi there");
    Program program;
    program.addVertexShader(vertexShader);
    program.addFragmentShader(fragmentShader);
    program.compile();

    VertexBuffer vertexBuffer(
        { glm::vec4{0,0,-1.1,1}, glm::vec4{0.5,0.5,0,1}, glm::vec4{0.5,0,0,1}, glm::vec4{1.0f,1.0f,0,1.0f},
          glm::vec4{1.0f,1.0f,1.0f,1.0f}, glm::vec4{1.0f,1.0f,1.0f,1.0f}, glm::vec4{1.0f,1.0f,1.0f,1.0f}, glm::vec4{1.0f,1.0f,1.0f,1.0f}});
    IndexBuffer indexBuffer({0,1,2});

    Mesh mesh(vertexBuffer, indexBuffer);

    while (!window.shouldClose()) {
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

////        program.bind();
////        vertexBuffer.bind();
////        glEnableVertexAttribArray(0);
////        glEnableVertexAttribArray(1);
////        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
////        auto colorsOffset = reinterpret_cast<void*>(vertexBuffer.size() * sizeof(vec4) / 2);
////        glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, colorsOffset);
////        indexBuffer.bind();
////        glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_SHORT, 0);

        mesh.draw(program);
        window.swap();
    }
    return 0;
}
