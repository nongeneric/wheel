#include <GL/glew.h>
#include <GLFW/glfw3.h>
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
#include <random>
#include <ctime>

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;
const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

template <typename T>
struct Distrib { static_assert(sizeof(T) && false, "bad type"); };
template <>
struct Distrib<float> {
    typedef typename std::uniform_real_distribution<float> type;
};
template <>
struct Distrib<unsigned> {
    typedef typename std::uniform_int_distribution<unsigned> type;
};

template <typename T>
class Random {
    typename Distrib<T>::type _distribution;
    std::mt19937 _engine;
    std::function<T()> _rndfunc;
public:
    Random(T from, T to)
        : _distribution(from, to)
    {
#ifndef DEBUG
        _engine.seed(time(NULL));
#endif
        _rndfunc = std::bind(_distribution, _engine);
    }
    T operator()() {
        return _rndfunc();
    }
};

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

bool isCompleted(ScaleAnimation& a) {
    return a._isCompleted;
}

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
    int getMouseButton(int button) {
        return glfwGetMouseButton(_window, button);
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

namespace rstd {
    template<typename T, typename Cont>
    void fill(Cont& cont, T val) {
        std::fill(std::begin(cont), std::end(cont), val);
    }
    template<typename Pred, typename Cont>
    bool any_of(Cont& cont, Pred pred) {
        return std::any_of(std::begin(cont), std::end(cont), pred);
    }
    template<typename Pred, typename Cont>
    bool all_of(Cont& cont, Pred pred) {
        return std::all_of(std::begin(cont), std::end(cont), pred);
    }
    template<typename Pred, typename Cont>
    typename Cont::iterator stable_partition(Cont& cont, Pred pred) {
        return std::stable_partition(std::begin(cont), std::end(cont), pred);
    }
    template<typename Cont, typename Iter>
    void copy(Cont& cont, Iter out) {
        std::copy(std::begin(cont), std::end(cont), out);
    }
}

enum class CellState {
    Shown, Hidden, Dying
};

struct BBox {
    int x, y, size;
};

namespace PieceType {
    enum t {
        I, T, count
    };
}

namespace PieceOrientation {
    enum t {
        Up, Right, Down, Left
    };
}

std::vector<PieceOrientation::t> OrientationCircle {
    PieceOrientation::Up,
    PieceOrientation::Right,
    PieceOrientation::Down,
    PieceOrientation::Left
};

using Line = std::vector<CellState>;
using State = std::vector<Line>;

auto _1 = CellState::Shown;
auto _0 = CellState::Hidden;

auto barUp = State {
    Line { _1, _1, _1, _1 },
    Line { _0, _0, _0, _0 },
    Line { _0, _0, _0, _0 },
    Line { _0, _0, _0, _0 },
};

auto barRight = State {
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
};

auto pieceTup = State {
    Line { _0, _1, _0 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceTright = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
};

auto pieceTdown = State {
    Line { _0, _0, _0 },
    Line { _1, _1, _1 },
    Line { _0, _1, _0 },
};

auto pieceTleft = State {
    Line { _0, _1, _0 },
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceJup = State {
    Line { _1, _1, _1 },
    Line { _0, _0, _1 },
    Line { _0, _0, _0 },
};

auto pieceJright = State {
    Line { _0, _0, _1 },
    Line { _0, _0, _1 },
    Line { _0, _1, _1 },
};

auto pieceJdown = State {
    Line { _0, _0, _0 },
    Line { _1, _0, _0 },
    Line { _1, _1, _1 },
};

auto pieceJleft = State {
    Line { _1, _1, _0 },
    Line { _1, _0, _0 },
    Line { _1, _0, _0 },
};

auto pieceLup = State {
    Line { _1, _1, _1 },
    Line { _1, _0, _0 },
    Line { _0, _0, _0 },
};

auto pieceLright = State {
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceLdown = State {
    Line { _0, _0, _1 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceLleft = State {
    Line { _1, _0, _0 },
    Line { _1, _0, _0 },
    Line { _1, _1, _0 },
};

auto pieceO = State {
    Line { _1, _1 },
    Line { _1, _1 },
};

auto pieceSup = State {
    Line { _0, _1, _1 },
    Line { _1, _1, _0 },
    Line { _0, _0, _0 },
};

auto pieceSright = State {
    Line { _1, _0, _0 },
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceZup = State {
    Line { _1, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceZright = State {
    Line { _0, _0, _1 },
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
};

State pieces[][4] = {
    { barUp, barRight, barUp, barRight },
    { pieceJup, pieceJright, pieceJdown, pieceJleft },
    { pieceLup, pieceLright, pieceLdown, pieceLleft },
    { pieceO, pieceO, pieceO, pieceO },
    { pieceSup, pieceSright, pieceSup, pieceSright },
    { pieceTup, pieceTright, pieceTdown, pieceTleft },
    { pieceZup, pieceZright, pieceZup, pieceZright },
};

class Tetris {
    int _hor, _vert;
    bool _nothingFalling = true;
    State _staticGrid;
    State _dynamicGrid;
    BBox _bbPiece;
    PieceType::t _piece;
    PieceOrientation::t _pieceOrientation;
    Random<unsigned> _random;
    State cut(State& source, unsigned posX, unsigned posY, unsigned size) {
        State res(size);
        for (size_t y = 0; y < size; ++y) {
            Line& sourceLine = source.at(y + posY);
            auto sourceLineBegin = begin(sourceLine) + posX;
            auto sourceLineEnd = begin(sourceLine) + posX + size;
            std::copy(sourceLineBegin,
                      sourceLineEnd,
                      std::back_inserter(res.at(y)));
            std::fill(sourceLineBegin,
                      sourceLineEnd,
                      CellState::Hidden);
        }
        return res;
    }
    void paste(State const& piece, State& state, unsigned posX, unsigned posY) {
        for (size_t y = 0; y < piece.size(); ++y) {
            rstd::copy(piece.at(y), begin(state.at(posY + y)) + posX);
        }
    }
    void drawPiece() {
        _piece = static_cast<PieceType::t>(_random());
        State piece = pieces[_piece][PieceOrientation::Up];
        _bbPiece = { 7, _vert - 4 - (int)piece.size(), 4 };
        paste(piece, _dynamicGrid, _bbPiece.x, _bbPiece.y);
    }
    void nextPiece() {
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
        drawPiece();
        _nothingFalling = false;
    }
    bool collision(State& state) {
        assert(_staticGrid.size() == state.size());
        for (int x = 0; x < _hor; ++x) {
            for (int y = 0; y < _vert; ++y) {
                if (state.at(y).at(x) == CellState::Shown &&
                    _staticGrid.at(y).at(x) == CellState::Shown)
                    return true;
            }
        }
        return false;
    }
    void stamp(State const& state) {
        for (int x = 0; x < _hor; ++x) {
            for (int y = 0; y < _vert; ++y) {
                if (state.at(y).at(x) == CellState::Shown) {
                    _staticGrid.at(y).at(x) = CellState::Shown;
                }
            }
        }
    }
    void kill() {
        std::for_each(begin(_staticGrid) + 4, end(_staticGrid), [](Line& line) {
            bool hit = rstd::all_of(line, [](CellState cell) {
                return cell == CellState::Shown;
            });
            if (hit) {
                rstd::fill(line, CellState::Dying);
            }
        });
    }
    void collect() {
        auto middle = rstd::stable_partition(_staticGrid, [](Line const& line) {
            return !rstd::all_of(line, [](CellState cell) {
                return cell == CellState::Dying;
            });
        });
        std::fill(middle, end(_staticGrid), Line(_hor, CellState::Hidden));
    }
    void drop() {
        collect();
        auto res = _dynamicGrid;
        std::copy(begin(res) + 1, end(res), begin(res));
        rstd::fill(res.at(_vert - 1), CellState::Hidden);

        if (collision(res)) {
            stamp(_dynamicGrid);
            _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
            _nothingFalling = true;
        } else {
            _dynamicGrid = res;
            _bbPiece.y -= 1;
        }
        kill();
    }
    State createState(int width, int height, CellState val) {
        State state;
        state.resize(height);
        for (Line& line : state) {
            line.resize(width);
            rstd::fill(line, val);
        }
        return state;
    }
    void movePieceHor(State& state, int offset) {
        auto box = cut(state, _bbPiece.x, _bbPiece.y, _bbPiece.size);
        paste(box, state, _bbPiece.x + offset, _bbPiece.y);
    }
    void moveHor(int offset) {
        if (_nothingFalling)
            return;
        auto state = _dynamicGrid;
        movePieceHor(state, offset);
        if (!collision(state)) {
            _dynamicGrid = state;
            _bbPiece.x += offset;
        }
    }
    PieceOrientation::t nextOrientation(PieceOrientation::t prev) {
        return OrientationCircle[(prev + 1) % OrientationCircle.size()];
    }
public:
    Tetris(int hor, int vert)
        : _hor(hor + 8),
          _vert(vert + 8),
          _random(0, PieceType::count - 1)
    {
        State inner = createState(hor, vert + 4, CellState::Hidden);
        _staticGrid = createState(_hor, _vert, CellState::Shown);
        paste(inner, _staticGrid, 4, 4);
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
    }
    CellState getState(int x, int y) {
        if (_dynamicGrid.at(y + 4).at(x + 4) == CellState::Shown)
            return CellState::Shown;
        return _staticGrid.at(y + 4).at(x + 4);
    }
    void step() {
        if (_nothingFalling)
            nextPiece();
        drop();
    }
    void moveRight() {
        moveHor(1);
    }
    void moveLeft() {
        moveHor(-1);
    }
    void rotate() {
        if (_bbPiece.y < 0)
            return;
        auto copy = _dynamicGrid;
        int rightSpike = std::max(0, _bbPiece.x + _bbPiece.size - (_hor - 4));
        int leftSpike = std::max(0, 4 - _bbPiece.x);
        int offset = leftSpike - rightSpike;
        movePieceHor(copy, offset);
        auto newOrient = nextOrientation(_pieceOrientation);
        paste(pieces[_piece][newOrient], copy, _bbPiece.x + offset, _bbPiece.y);
        if (!collision(copy)) {
            _dynamicGrid = copy;
            _pieceOrientation = newOrient;
            _bbPiece.x += offset;
        }
    }
};

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
