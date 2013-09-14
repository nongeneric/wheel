#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <boost/log/trivial.hpp>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

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
        _window = glfwCreateWindow(600, 600, title.c_str(), NULL, NULL);
        //_window = glfwCreateWindow(1920, 1200, title.c_str(), glfwGetPrimaryMonitor(), NULL);
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
    int getKey(int key) {
        return glfwGetKey(_window, key);
    }
    glm::vec2 getCursorPos() {
        double x, y;
        glfwGetCursorPos(_window, &x, &y);
        return glm::vec2(x, y);
    }
    glm::vec2 getFramebufferSize() {
        int width, height;
        glfwGetFramebufferSize(_window, &width, &height);
        return glm::vec2(width, height);
    }
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
    void link() {
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
    GLuint getUniformLocation(std::string const& name) {
        GLuint loc = glGetUniformLocation(_program, name.c_str());
        if (loc == (GLuint)-1) {
            throw std::runtime_error("uniform get location error");
        }
        return loc;
    }
    void setUniform(GLuint location, glm::mat4 const& matrix) {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(matrix));
        GLenum error = glGetError();
        if (error != GL_NO_ERROR && error != GL_INVALID_ENUM ) {
            throw std::runtime_error("uniform set error");
        }
    }
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
    glm::mat4 _pos;
public:
    Mesh(VertexBuffer vertices, IndexBuffer indices) {
        _indicesCount = indices.size();
        BindLock<VAO> vaoLock(_vao);
        init_vao(vertices);
        indices.bind();
    }
    // assumes a program is bound, doesn't manage its position
    void draw() {
        BindLock<VAO> vaoLock(_vao);
        glDrawElements(GL_TRIANGLES, _indicesCount, GL_UNSIGNED_SHORT, 0);
    }
    void setPos(glm::mat4 pos) {
        _pos = pos;
    }
    glm::mat4 const& getPos() const {
        return _pos;
    }
};

glm::mat4 getViewMatrix(glm::vec3 const& angles, glm::vec3 const& pos) {
    glm::mat4 rotation = glm::rotate( glm::mat4(), -angles.x, glm::vec3 { 1, 0, 0 } ) *
                         glm::rotate( glm::mat4(), -angles.y, glm::vec3 { 0, 1, 0 } ) *
                         glm::rotate( glm::mat4(), -angles.z, glm::vec3 { 0, 0, 1 } );
    glm::mat4 translation = glm::translate( {}, -pos );
    return translation * rotation;
}

Mesh genCube() {
    VertexBuffer vertices{{
        glm::vec4 { 0.5f, 0.5f, 0.0f, 1.0f },
        glm::vec4 { 0.5f, -0.5f, 0.0f, 1.0f },
        glm::vec4 { -0.5f, -0.5f, 0.0f, 1.0f },
        glm::vec4 { -0.5f, 0.5f, 0.0f, 1.0f },
        glm::vec4 { 0.5f, 0.5f, 1.0f, 1.0f },
        glm::vec4 { 0.5f, -0.5f, 1.0f, 1.0f },
        glm::vec4 { -0.5f, -0.5f, 1.0f, 1.0f },
        glm::vec4 { -0.5f, 0.5f, 1.0f, 1.0f },
        // colors
        glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
        glm::vec4 { 0.0f, 1.0f, 0.0f, 1.0f },
        glm::vec4 { 0.0f, 0.0f, 1.0f, 1.0f },
        glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
        glm::vec4 { 0.0f, 1.0f, 0.0f, 1.0f },
        glm::vec4 { 0.0f, 0.0f, 1.0f, 1.0f },
        glm::vec4 { 1.0f, 0.0f, 0.0f, 1.0f },
        glm::vec4 { 0.0f, 1.0f, 0.0f, 1.0f },
    }};
    enum { b1, b2, b3, b4, u1, u2, u3, u4 };
    IndexBuffer indices{{
        b1, u1, b2, b2, u2, u1,
        b2, u2, b3, b3, u3, u2,
        b3, u3, b4, b4, u4, u3,
        b4, u4, b1, b1, u1, u4,
        b1, b2, b4, b4, b3, b2,
        u1, u2, u4, u4, u3, u2
    }};
    return Mesh(vertices, indices);
}

Mesh genZXPlato(glm::vec4 color, GLfloat size) {
    VertexBuffer vertices{{
        glm::vec4 { 0.0f, 0.0f, 0.0f, 1.0f },
        glm::vec4 { size, 0.0f, 0.0f, 1.0f },
        glm::vec4 { size, 0.0f, size, 1.0f },
        glm::vec4 { 0.0f, 0.0f, size, 1.0f },
        color, color, color, color
    }};
    enum { b1, b2, b3, b4 };
    IndexBuffer indices{{
        b1, b2, b3, b3, b1, b4
    }};
    return Mesh(vertices, indices);
}

std::string vertexShader =
        "#version 330\n"

        "uniform mat4 mvp;\n"
        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec4 color;\n"
        "smooth out vec4 f_color;\n"
        "void main() {\n"
        "    gl_Position = mvp * position;\n"
        "    f_color = color;"
        "}\n"
        ;

std::string fragmentShader =
        "#version 330\n"

        "smooth in vec4 f_color;\n"
        "out vec4 outputColor;\n"
        "void main() {\n"
        "   outputColor = f_color;\n"
        "}"
        ;

int main() {
    Window window("hi there");
    Program program;
    program.addVertexShader(vertexShader);
    program.addFragmentShader(fragmentShader);
    program.link();
    const GLuint U_MVP = program.getUniformLocation("mvp");

    glm::vec4 red { 1, 0, 0, 1 };
    glm::vec4 green { 0, 1, 0, 1 };
    glm::vec4 blue { 0, 0, 1, 1 };
    glm::vec4 gray { 0.5, 0.5, 0.5, 1 };

    GLfloat plato_size = 20.0f;
    std::vector<Mesh> meshes {
        genCube(), genCube(), genCube(),
        genZXPlato(red, plato_size),
        genZXPlato(green, plato_size),
        genZXPlato(blue, plato_size),
        genZXPlato(gray, plato_size)
    };

    enum { cube1, cube2, cube3, red_plato, green_plato, blue_plato, gray_plato };

    meshes[cube1].setPos(
        glm::translate( glm::mat4(), glm::vec3 { 5, 0.5, 5 } ) *
        glm::rotate( glm::mat4(), 40.0f, glm::vec3 {0.0f, 1.0f, 0.0f} )
    );
    meshes[cube2].setPos(
        glm::translate( glm::mat4(), glm::vec3 { 0, 0.5, 5 } )
    );
    meshes[cube3].setPos(
        glm::translate( glm::mat4(), glm::vec3 { 10, 0.5, 5 } )
    );
    meshes[red_plato].setPos(
        glm::translate( glm::mat4(), glm::vec3 { 0, 0, 0 } )
    );
    meshes[green_plato].setPos(
        glm::translate( glm::mat4(), glm::vec3 { 0, 0, -plato_size } )
    );
    meshes[blue_plato].setPos(
        glm::translate( glm::mat4(), glm::vec3 { -plato_size, 0, 0 } )
    );
    meshes[gray_plato].setPos(
        glm::translate( glm::mat4(), glm::vec3 { -plato_size, 0, -plato_size } )
    );

    glm::vec3 cameraAngles{-5, -45, 0};
    glm::vec3 cameraPos{0, 5, 25};

    glEnable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1);
    glm::vec2 cursor{ 300, 300 };
    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec2 size = window.getFramebufferSize();
        glViewport(0, 0, size.x, size.y);
        auto proj = glm::perspective(30.0f, size.x / size.y, 1.0f, 1000.0f);
        glm::mat4 vpMatrix = proj * getViewMatrix(
            cameraAngles,
            cameraPos
        );

        BindLock<Program> programLock(program);
        for (Mesh& mesh : meshes) {
            glm::mat4 mvpMatrix = vpMatrix * mesh.getPos();
            program.setUniform(U_MVP, mvpMatrix);
            mesh.draw();
        }

        window.swap();
        bool leftPressed = window.getKey(GLFW_KEY_LEFT) == GLFW_PRESS;
        bool rightPressed = window.getKey(GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool upPressed = window.getKey(GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = window.getKey(GLFW_KEY_DOWN) == GLFW_PRESS;
        cameraPos += glm::vec3 {
            -0.5 * leftPressed + 0.5 * rightPressed,
            0,
            -0.5 * upPressed + 0.5 * downPressed
        };

        glm::vec2 delta = window.getCursorPos() - cursor;
        cursor = window.getCursorPos();
        if (delta.x > 20 || delta.y > 20)
            continue;
        cameraAngles += glm::vec3 { 0.5 * delta.y, -0.5 * delta.x, 0 };
    }
    return 0;
}
