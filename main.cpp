#include "Random.h"
#include "Window.h"
#include "Program.h"
#include "Tetris.h"

#include <boost/log/trivial.hpp>
#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <boost/any.hpp>
#include <boost/chrono.hpp>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <algorithm>

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;
const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

class Mesh;
void setScale(Mesh& mesh, glm::vec3 scale);

class ScaleAnimation {
    fseconds _elapsed;
    fseconds _duration;
    GLfloat _from, _to;
    bool _isCompleted = true;
public:
    ScaleAnimation() = default;
    ScaleAnimation(fseconds duration, GLfloat from, GLfloat to)
        : _duration(duration), _from(from), _to(to), _isCompleted(false) { }
    void animate(fseconds dt, std::function<Mesh&()> mesh) {
        if (_isCompleted)
            return;
        _elapsed += dt;
        GLfloat factor;
        if (_elapsed > _duration) {
            _isCompleted = true;
            factor = _to;
        } else {
            factor = _from + (_to - _from) * (_elapsed / _duration);
        }
        setScale(mesh(), glm::vec3 { factor, factor, factor });
    }
    friend bool isCompleted(ScaleAnimation& a);
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

class Mesh {
    VAO _vao;
    unsigned _indicesCount;
    glm::mat4 _pos;
    glm::mat4 _scale;
    ScaleAnimation _animation;
public:
    Mesh(VertexBuffer vertices, IndexBuffer indices) {
        _indicesCount = indices.size();
        BindLock<VAO> vaoLock(_vao);
        init_vao(vertices);
        indices.bind();
    }
    // assumes a program is bound, doesn't manage its position
    friend void draw(Mesh& mesh, int mvp_location, glm::mat4 vp, Program& program);
    friend void setPos(Mesh& mesh, glm::vec3 pos);
    friend void setScale(Mesh& mesh, glm::vec3 scale);
    friend void setAnimation(Mesh& mesh, ScaleAnimation a);
    friend void animate(Mesh& mesh, fseconds dt);
    friend glm::mat4 getTransform(Mesh& mesh);
    glm::mat4 const& getPos() const {
        return _pos;
    }
};

glm::mat4 getTransform(Mesh& mesh) {
    return mesh._pos * mesh._scale;
}

void draw(Mesh& mesh, int mvp_location, glm::mat4 vp, Program& program) {
    BindLock<VAO> vaoLock(mesh._vao);
    glm::mat4 mvp = vp * getTransform(mesh);
    program.setUniform(mvp_location, mvp);
    glDrawElements(GL_TRIANGLES, mesh._indicesCount, GL_UNSIGNED_SHORT, 0);
}

void setPos(Mesh& mesh, glm::vec3 pos) {
    mesh._pos = glm::translate( {}, pos );
}

void setScale(Mesh& mesh, glm::vec3 scale) {
    mesh._scale = glm::scale({}, scale);
}

void setAnimation(Mesh& mesh, ScaleAnimation a) {
    mesh._animation = a;
}

void animate(Mesh& mesh, fseconds dt) {
    mesh._animation.animate(dt, [&]() -> Mesh& { return mesh; });
}

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

class Trunk {
    std::vector<Mesh> _cubes;
    glm::mat4 _pos;
    int _hor, _vert;
    Mesh& at(int x, int y) {
        return _cubes.at(_hor * y + x);
    }
public:
    Trunk(int hor, int vert)
        : _hor(hor), _vert(vert)
    {
        for (int y = 0; y < vert; ++y) {
            for (int x = 0; x < hor; ++x) {
                Mesh cube = genCube();
                setPos(cube, glm::vec3 {x * 1.3, y * 1.3, 0});
                _cubes.push_back(cube);
            }
        }
    }
    void animateDestroy(int x, int y, fseconds duration) {
        setAnimation(at(x, y), ScaleAnimation(duration, 1, 0));
    }
    void hide(int x, int y) {
        setScale(at(x, y), glm::vec3 {0, 0, 0});
    }
    void show(int x, int y) {
        setScale(at(x, y), glm::vec3 {1, 1, 1});
    }
    friend void animate(Trunk& trunk, fseconds dt);
    friend void draw(Trunk&, int, glm::mat4, Program&);
    friend void setPos(Trunk&, glm::vec3);
    friend glm::mat4 const& getPos(Trunk&);
};

void draw(Trunk& t, int mvp_location, glm::mat4 vp, Program& program) {
    for (Mesh& m : t._cubes)
        ::draw(m, mvp_location, vp * t._pos, program);
}

void setPos(Trunk& trunk, glm::vec3 pos) {
    trunk._pos = glm::translate({}, pos);
}

void animate(Trunk& trunk, fseconds dt) {
    for (Mesh& m : trunk._cubes)
        animate(m, dt);
}

class MeshWrapper {
    struct concept {
        virtual ~concept() = default;
        virtual concept* copy_() = 0;
        virtual void draw_(int mvp_location, glm::mat4 vp, Program& program) = 0;
        virtual void setPos_(glm::vec3) = 0;
        virtual void animate_(fseconds dt) = 0;
    };
    template <typename T>
    struct model : concept {
        model(T m) : data_(std::move(m)) { }
        virtual concept* copy_() override {
            return new model(*this);
        }
        virtual void draw_(int mvp_location, glm::mat4 vp, Program& program) override {
            ::draw(data_, mvp_location, vp, program);
        }
        virtual void setPos_(glm::vec3 pos) {
            ::setPos(data_, pos);
        }
        virtual void animate_(fseconds dt) {
            ::animate(data_, dt);
        }
        T data_;
    };
    std::unique_ptr<concept> _ptr;
public:
    template <typename T>
    MeshWrapper(T x) : _ptr(new model<T>(std::move(x))) { }
    MeshWrapper(MeshWrapper const& w) : _ptr(w._ptr->copy_()) { }
    MeshWrapper(MeshWrapper&& w) : _ptr(std::move(w._ptr)) { }
    friend void draw(MeshWrapper& w, int mvp_location, glm::mat4 vp, Program& program) {
        w._ptr->draw_(mvp_location, vp, program);
    }
    friend void setPos(MeshWrapper& w, glm::vec3 pos) {
        w._ptr->setPos_(pos);
    }
    friend void animate(MeshWrapper& w, fseconds dt) {
        w._ptr->animate_(dt);
    }
    template <typename T>
    T& obj() {
        return dynamic_cast<model<T>*>(_ptr.get())->data_;
    }
};

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

enum { red_plato, green_plato, blue_plato, gray_plato, trunk };
std::vector<MeshWrapper> genMeshes() {
    glm::vec4 red { 1, 0, 0, 1 };
    glm::vec4 green { 0, 1, 0, 1 };
    glm::vec4 blue { 0, 0, 1, 1 };
    glm::vec4 gray { 0.5, 0.5, 0.5, 1 };
    GLfloat plato_size = 20.0f;
    std::vector<MeshWrapper> meshes {
        genZXPlato(red, plato_size),
        genZXPlato(green, plato_size),
        genZXPlato(blue, plato_size),
        genZXPlato(gray, plato_size),
        Trunk(g_TetrisHor, g_TetrisVert)
    };

    setPos(meshes[red_plato], glm::vec3 { 0, 0, 0 } );
    setPos(meshes[green_plato], glm::vec3 { 0, 0, -plato_size });
    setPos(meshes[blue_plato], glm::vec3 { -plato_size, 0, 0 });
    setPos(meshes[gray_plato], glm::vec3 { -plato_size, 0, -plato_size });
    Trunk& tr = meshes[trunk].obj<Trunk>();
    setPos(tr, glm::vec3 { -10, 0, 0 } );
    return meshes;
}

fseconds copyState(Tetris& tetris, Trunk& trunk) {
    bool dying = false;
    fseconds duration(1.0f);
    for (int x = 0; x < g_TetrisHor; ++x) {
        for (int y = 0; y < g_TetrisVert; ++y) {
            switch (tetris.getState(x, y)) {
            case CellState::Shown:
                trunk.show(x, y);
                break;
            case CellState::Hidden:
                trunk.hide(x, y);
                break;
            case CellState::Dying:
                dying = true;
                trunk.animateDestroy(x, y, duration);
                break;
            default: assert(false);
            }
        }
    }
    return dying ? duration : fseconds();
}

int main() {
    Window window("wheel");
    Program program;
    program.addVertexShader(vertexShader);
    program.addFragmentShader(fragmentShader);
    program.link();
    const GLuint U_MVP = program.getUniformLocation("mvp");

    auto meshes = genMeshes();

    Tetris tetris(g_TetrisHor, g_TetrisVert);

    glm::vec3 cameraAngles{-5, 0, 0};
    glm::vec3 cameraPos{0, 2, 100};

    glEnable(GL_DEPTH_TEST);
    glClearColor(0,0,0,1);
    glm::vec2 cursor{ 300, 300 };
    auto past = chrono::high_resolution_clock::now();
    fseconds elapsed;

    int prevKP3State = GLFW_RELEASE;
    int prevKP1State = GLFW_RELEASE;
    int prevKP6State = GLFW_RELEASE;

    fseconds wait;

    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec2 size = window.getFramebufferSize();
        glViewport(0, 0, size.x, size.y);
        auto proj = glm::perspective(30.0f, size.x / size.y, 1.0f, 1000.0f);
        glm::mat4 vpMatrix = proj * getViewMatrix(
            cameraAngles,
            cameraPos
        );

        auto now = chrono::high_resolution_clock::now();
        fseconds dt = chrono::duration_cast<fseconds>(now - past);
        elapsed += dt;
        wait -= dt;
        past = now;

        BindLock<Program> programLock(program);
        for (MeshWrapper& mesh : meshes) {
            animate(mesh, dt);
            draw(mesh, U_MVP, vpMatrix, program);
        }

        window.swap();

        if (wait > fseconds())
            continue;

        if (elapsed > fseconds(0.5f)) {
            tetris.step();
            elapsed = fseconds();
        }

        bool leftPressed = window.getKey(GLFW_KEY_LEFT) == GLFW_PRESS;
        bool rightPressed = window.getKey(GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool upPressed = window.getKey(GLFW_KEY_UP) == GLFW_PRESS;
        bool downPressed = window.getKey(GLFW_KEY_DOWN) == GLFW_PRESS;
        cameraPos += glm::vec3 {
            -0.5 * leftPressed + 0.5 * rightPressed,
            0,
            -0.5 * upPressed + 0.5 * downPressed
        };
        bool mouseLeftPressed = window.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mouseLeftPressed) {
            glm::vec2 delta = window.getCursorPos() - cursor;
            cameraAngles += glm::vec3 { -0.5 * delta.y, -0.5 * delta.x, 0 };
        }
        if (window.getKey(GLFW_KEY_KP_3) == GLFW_RELEASE && prevKP3State == GLFW_PRESS) {
            tetris.moveRight();
        }
        if (window.getKey(GLFW_KEY_KP_1) == GLFW_RELEASE && prevKP1State == GLFW_PRESS) {
            tetris.moveLeft();
        }
        if (window.getKey(GLFW_KEY_KP_6) == GLFW_RELEASE && prevKP6State == GLFW_PRESS) {
            tetris.rotate();
        }
        if (window.getKey(GLFW_KEY_KP_2)) {
            tetris.step();
        }
        prevKP3State = window.getKey(GLFW_KEY_KP_3);
        prevKP1State = window.getKey(GLFW_KEY_KP_1);
        prevKP6State = window.getKey(GLFW_KEY_KP_6);
        cursor = window.getCursorPos();

        wait = copyState(tetris, meshes[trunk].obj<Trunk>());
    }
    return 0;
}
