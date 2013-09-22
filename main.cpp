#include "Random.h"
#include "Window.h"
#include "Program.h"
#include "Tetris.h"
#include "Text.h"

#include <cstring>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <boost/chrono.hpp>
#include <boost/log/trivial.hpp>
#include <boost/format.hpp>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <map>

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;
const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

template <typename FormatType>
void appendFormat(FormatType& fmt) { }

template <typename FormatType, typename T, typename... Ts>
void appendFormat(FormatType& fmt, T arg, Ts... args) {
    appendFormat(fmt % arg, args...);
}

template <typename StrType, typename... Ts>
std::string vformat(StrType formatString, Ts... ts) {
    boost::format fmt(formatString);
    appendFormat(fmt, ts...);
    return str(fmt);
}

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
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

typedef Buffer<Vertex, GL_ARRAY_BUFFER> VertexBuffer;
typedef Buffer<GLshort, GL_ELEMENT_ARRAY_BUFFER> IndexBuffer;

void init_vao(VertexBuffer buffer) {
    buffer.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)20);
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
    friend void draw(Mesh& mesh, int mv_location, int mvp_location, glm::mat4 vp, Program& program);
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

void draw(Mesh& mesh, int world_location, int mvp_location, glm::mat4 vp, Program& program) {
    BindLock<VAO> vaoLock(mesh._vao);
    glm::mat4 mvp = vp * getTransform(mesh);
    program.setUniform(mvp_location, mvp);
    program.setUniform(world_location, getTransform(mesh));
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

Mesh genZXPlato(GLfloat tex, GLfloat size) {
    VertexBuffer vertices{{
        {glm::vec3 {0.0f, 0.0f, 0.0f}, glm::vec2 {tex, tex}},
        {glm::vec3 {size, 0.0f, 0.0f}, glm::vec2 {0, 0}},
        {glm::vec3 {size, 0.0f, size}, glm::vec2 {tex, tex}},
        {glm::vec3 {0.0f, 0.0f, size}, glm::vec2 {0, 0}},
    }};
    enum { b1, b2, b3, b4 };
    IndexBuffer indices{{
        b1, b2, b3, b3, b1, b4
    }};
    return Mesh(vertices, indices);
}

Mesh loadMesh() {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("cube.ply",
         aiProcess_CalcTangentSpace       |
         aiProcess_Triangulate            |
         aiProcess_JoinIdenticalVertices);
    if (!scene) {
        BOOST_LOG_TRIVIAL(error) << importer.GetErrorString();
        throw std::runtime_error("can't load a mesh");
    }
    assert(scene->HasMeshes());
    assert(scene->mNumMeshes == 1);
    std::vector<Vertex> vertices;
    aiMesh* mesh = scene->mMeshes[0];
    assert(mesh->HasNormals());
    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
        vertices.push_back({
            glm::vec3(mesh->mVertices[i].x,
                      mesh->mVertices[i].y,
                      mesh->mVertices[i].z),
            glm::vec2(),
            glm::vec3(mesh->mNormals[i].x,
                      mesh->mNormals[i].y,
                      mesh->mNormals[i].z)});
    }
    std::vector<short> indices;
    for (size_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }
    return Mesh(VertexBuffer(vertices), IndexBuffer(indices));
}

const float cubeSpace = 0.2f;
class Trunk {
    std::vector<Mesh> _cubes;
    std::vector<Mesh> _border;
    glm::mat4 _pos;
    int _hor, _vert;
    bool _drawBorder;
    Mesh& at(int x, int y) {
        return _cubes.at(_hor * y + x);
    }
public:
    Trunk(int hor, int vert, bool drawBorder)
        : _hor(hor), _vert(vert), _drawBorder(drawBorder)
    {
        float xOffset = (1.0 * (hor + 2) + cubeSpace * (hor + 1) - 0.5) / -2.0f;
        for (int y = 0; y < vert; ++y) {
            for (int x = 0; x < hor; ++x) {
                Mesh cube = loadMesh();
                float xPos = xOffset + (cubeSpace + 1) * (x + 1);
                float yPos = y * (1 + cubeSpace) + cubeSpace;
                setPos(cube, glm::vec3 { xPos, yPos, 0 });
                _cubes.push_back(cube);
            }
        }
        if (!_drawBorder)
            return;
        for (int y = 0; y < vert; ++y) {
            Mesh left = loadMesh();
            setPos(left, glm::vec3 { xOffset, y * (1 + cubeSpace) + cubeSpace, 0 });
            _border.push_back(left);
            Mesh right = loadMesh();
            float xPos = xOffset + (hor + 1) * (1 + cubeSpace);
            float yPos = y * (1 + cubeSpace) + cubeSpace;
            setPos(right, glm::vec3 { xPos, yPos, 0 });
            _border.push_back(right);
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
    friend void draw(Trunk&, int, int, glm::mat4, Program&);
    friend void setPos(Trunk&, glm::vec3);
    friend glm::mat4 const& getPos(Trunk&);
};

void draw(Trunk& t, int mv_location, int mvp_location, glm::mat4 vp, Program& program) {
    for (Mesh& m : t._cubes)
        ::draw(m, mv_location, mvp_location, vp * t._pos, program);
    for (Mesh& m : t._border)
        ::draw(m, mv_location, mvp_location, vp * t._pos, program);
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
        virtual void draw_(int mv_location, int mvp_location, glm::mat4 vp, Program& program) = 0;
        virtual void setPos_(glm::vec3) = 0;
        virtual void animate_(fseconds dt) = 0;
    };
    template <typename T>
    struct model : concept {
        model(T m) : data_(std::move(m)) { }
        virtual concept* copy_() override {
            return new model(*this);
        }
        virtual void draw_(int mv_location, int mvp_location, glm::mat4 vp, Program& program) override {
            ::draw(data_, mv_location, mvp_location, vp, program);
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
    friend void draw(MeshWrapper& w, int mv_location, int mvp_location, glm::mat4 vp, Program& program) {
        w._ptr->draw_(mv_location, mvp_location, vp, program);
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
        "uniform mat4 world;\n"
        "layout(location = 0) in vec4 position;\n"
        "layout(location = 1) in vec2 texCoord;\n"
        "layout(location = 2) in vec3 normal;\n"
        "out vec2 f_texCoord;\n"
        "out vec3 f_normal;\n"
        "void main() {\n"
        "    gl_Position = mvp * position;\n"
        "    f_texCoord = texCoord;\n"
        "    f_normal = (world * vec4(normal, 0.0)).xyz;\n"
        "}\n"
        ;

std::string fragmentShader =
        "#version 330\n"

        "in vec2 f_texCoord;\n"
        "in vec3 f_normal;\n"
        "out vec4 outputColor;\n"
        "uniform sampler2D gSampler;\n"
        "uniform float ambient;\n"
        // white color
        "vec4 getDiffuseFactor(vec3 direction, vec4 baseColor) {\n"
        "   float factor = dot(normalize(f_normal), -direction);\n"
        "   vec4 color;\n"
        "   if (factor > 0) {\n"
        "       color = baseColor * factor;\n"
        "   } else {\n"
        "       color = vec4(0.0, 0.0, 0.0, 0.0);\n"
        "   }\n"
        "   return color;\n"
        "}\n"
        "void main() {\n"
        "   vec4 source1 = getDiffuseFactor(vec3(-1.0, 0.0, -1.0), vec4(1,1,1,0.6));\n"
        "   vec4 source2 = getDiffuseFactor(vec3(1.0, 0.0, 1.0), vec4(1,0,0,0.4));\n"
        "   outputColor = texture2D(gSampler, f_texCoord.st) * (ambient + source1 + source2);\n"
        "}"
        ;

enum { red_plato, green_plato, blue_plato, gray_plato, trunk, nextPieceTrunk };
std::vector<MeshWrapper> genMeshes() {
    GLfloat plato_size = 20.0f;
    std::vector<MeshWrapper> meshes {
        genZXPlato(0.0f, plato_size),
        genZXPlato(1.0f, plato_size),
        genZXPlato(0.0f, plato_size),
        genZXPlato(1.0f, plato_size),
        Trunk(g_TetrisHor, g_TetrisVert, true),
        Trunk(4, 4, false)
    };

    setPos(meshes[red_plato], glm::vec3 { 0, 0, 0 } );
    setPos(meshes[green_plato], glm::vec3 { 0, 0, -plato_size });
    setPos(meshes[blue_plato], glm::vec3 { -plato_size, 0, 0 });
    setPos(meshes[gray_plato], glm::vec3 { -plato_size, 0, -plato_size });
    Trunk& tr = meshes[nextPieceTrunk].obj<Trunk>();
    setPos(tr, glm::vec3 { 12, 15, 0 } );
    return meshes;
}

fseconds copyState(
        Tetris& tetris,
        std::function<CellState(Tetris&, int x, int y)> getter,
        int xMax,
        int yMax,
        Trunk& trunk)
{
    bool dying = false;
    fseconds duration(0.7f);
    for (int x = 0; x < xMax; ++x) {
        for (int y = 0; y < yMax; ++y) {
            switch (getter(tetris, x, y)) {
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

Program createBitmapProgram() {
    Program res;
    res.addVertexShader(
        "#version 330\n"
        "layout(location = 0) in vec4 pos;\n"
        "layout(location = 1) in vec2 uv;\n"
        "uniform mat4 transform;\n"
        "out vec2 f_uv;\n"
        "void main() {\n"
        "    gl_Position = transform * pos;\n"
        "    f_uv = uv;\n"
        "}\n"
    );
    res.addFragmentShader(
        "#version 330\n"
        "in vec2 f_uv;\n"
        "uniform sampler2D sampler;\n"
        "out vec4 outputColor;\n"
        "void main() {\n"
        "   outputColor = texture2D(sampler, f_uv);\n"
        "}"
    );
    res.link();
    return res;
}

class Texture {
    GLuint _tex;
public:
    Texture() {
        glGenTextures(1, &_tex);
        BindLock<Texture> lock(*this);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    }
    void setImage(void* buffer, unsigned width, unsigned height) {
        BindLock<Texture> lock(*this);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
    }
    void bind() {
        glBindTexture(GL_TEXTURE_2D, _tex);
        glActiveTexture(_tex);
    }
    void unbind() {
        glBindTexture(GL_TEXTURE_2D, 0);
    }
};

class HudElem {
    Texture _tex;
    VAO _vao;
    VertexBuffer _vertices;
    IndexBuffer _indices;
    Program _program;
    GLuint _sampler;
    GLuint _transformUniform;
    glm::mat4 _transform;
    VertexBuffer getVertices() {
        return VertexBuffer {{
            {glm::vec3 { -1, -1, 0.5 }, glm::vec2 { 0, 0 }},
            {glm::vec3 { 1, -1, 0.5 }, glm::vec2 { 1, 0 }},
            {glm::vec3 { -1, 1, 0.5 }, glm::vec2 { 0, 1 }},
            {glm::vec3 { 1, 1, 0.5 }, glm::vec2 { 1, 1 }}
        }};
    }
    IndexBuffer getIndices() {
        return IndexBuffer{{
            0, 1, 3, 0, 2, 3
        }};
    }
    void initVao() {
        BindLock<VAO> lock(_vao);
        _vertices.bind();
        _indices.bind();
        _tex.bind();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    }
public:
    HudElem() : _vertices(getVertices()),
                _indices(getIndices()),
                _program(createBitmapProgram())
    {
        _sampler = _program.getUniformLocation("sampler");
        _transformUniform = _program.getUniformLocation("transform");
        initVao();
    }
    void setBitmap(void* buffer, unsigned width, unsigned height, unsigned x, unsigned y, unsigned portX, unsigned portY) {
        _tex.setImage(buffer, width, height);
        float normWidth = (float)width / portX;
        float normHeight = (float)height / portY;
        auto scale = glm::scale( {}, glm::vec3 {normWidth, normHeight, 1});
        float normDx = (float)x / portX * 2;
        float normDy = (float)y / portY * 2;
        auto translate = glm::translate( {}, glm::vec3 {normDx + normWidth - 1, normDy + normHeight - 1, 0});
        _transform = translate * scale;
    }
    void draw() {
        BindLock<Program> programLock(_program);
        _program.setUniform(_sampler, 0);
        _program.setUniform(_transformUniform, _transform);
        BindLock<Texture> texLock(_tex);
        BindLock<VAO> vaoLock(_vao);
        glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, 0);
    }
};

class Keyboard {
    struct ButtonState {
        int state;
        fseconds elapsed;
        fseconds repeat;
    };
    fseconds _repeatTime = fseconds(0.3f);
    std::map<int, ButtonState> _prevStates;
    std::map<int, std::function<void()>> _downHandlers;
    std::map<int, std::function<void()>> _repeatHandlers;
    std::map<int, bool> _activeRepeats;
    Window* _window;
    void invokeHandler(int key, std::map<int, std::function<void()>> const& handlers) {
        auto it = handlers.find(key);
        if (it != end(handlers)) {
            it->second();
        }
    }
public:
    Keyboard(Window* window) : _window(window) {  }
    void advance(fseconds dt) {
        for (auto& pair : _prevStates) {
            int curState = _window->getKey(pair.first);
            int prevState = pair.second.state;
            if (curState == GLFW_PRESS && prevState == GLFW_RELEASE) {
                invokeHandler(pair.first, _downHandlers);
                invokeHandler(pair.first, _repeatHandlers);
                _activeRepeats[pair.first] = true;
                pair.second.elapsed = fseconds();
            }
            if (curState == GLFW_PRESS &&
                prevState == GLFW_PRESS &&
                pair.second.elapsed > pair.second.repeat &&
                _activeRepeats[pair.first])
            {
                pair.second.elapsed = fseconds();
                invokeHandler(pair.first, _repeatHandlers);
            }
            pair.second = { _window->getKey(pair.first),
                            pair.second.elapsed + dt,
                            pair.second.repeat };
        }
    }

    void onDown(int key, std::function<void()> handler) {
        auto it = _prevStates.find(key);
        if (it == end(_prevStates)) {
            _prevStates[key] = { GLFW_RELEASE };
        }
        _downHandlers[key] = handler;
    }

    void onRepeat(int key, fseconds every, std::function<void()> handler) {
        _prevStates[key] = { GLFW_RELEASE, fseconds(), every };
        _repeatHandlers[key] = handler;
        _activeRepeats[key] = true;
    }

    void stopRepeats(int key) {
        if (_window->getKey(key) == GLFW_PRESS) {
            _activeRepeats[key] = false;
        }
    }
};

int main() {
    Window window("wheel");
    //Program bitmapProgram = createBitmapProgram();
    Program program;
    program.addVertexShader(vertexShader);
    program.addFragmentShader(fragmentShader);
    program.link();
    const GLuint U_MVP = program.getUniformLocation("mvp");
    const GLuint U_WORLD = program.getUniformLocation("world");
    const GLuint U_GSAMPLER = program.getUniformLocation("gSampler");
    const GLuint U_AMBIENT = program.getUniformLocation("ambient");
    //const GLuint U_DIFF_DIRECTION = program.getUniformLocation("diffDirection");

    std::vector<MeshWrapper> meshes = genMeshes();

    Tetris tetris(g_TetrisHor, g_TetrisVert);

    glm::vec3 cameraAngles{0, 0, 0};
    glm::vec3 cameraPos{0, 15, 60};

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glfwSwapInterval(1); // vsync on
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0,0);
    glm::vec2 cursor{ 300, 300 };
    auto past = chrono::high_resolution_clock::now();
    fseconds elapsed;
    fseconds wait;

    Texture tex;
    std::vector<unsigned> color { 0xFFFFFFFF, 0xFF000000 };
    tex.setImage(color.data(), 2, 1);

    HudElem hudLines, hudScore;
    Text text;

    fseconds delay = fseconds(1.0f);

    Keyboard keys(&window);
    bool keysInit = false;
    bool nextPiece = false;
    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec2 size = window.getFramebufferSize();
        glViewport(0, 0, size.x, size.y);

        std::string lines = vformat("Lines: %d", tetris.getStats().lines);
        auto linesText = text.renderText(lines, size.y * 0.05);
        unsigned linesHeight = FreeImage_GetHeight(linesText.get());
        hudLines.setBitmap(
            FreeImage_GetBits(linesText.get()),
            FreeImage_GetWidth(linesText.get()),
            linesHeight,
            0, size.y - linesHeight, size.x, size.y
        );

        std::string score = vformat("Score: %d", tetris.getStats().score);
        auto scoreText = text.renderText(score, size.y * 0.05);
        hudScore.setBitmap(
            FreeImage_GetBits(scoreText.get()),
            FreeImage_GetWidth(scoreText.get()),
            FreeImage_GetHeight(scoreText.get()),
            0, size.y - FreeImage_GetHeight(scoreText.get()) - linesHeight, size.x, size.y
        );

        auto proj = glm::perspective(30.0f, size.x / size.y, 1.0f, 1000.0f);
        glm::mat4 vpMatrix = proj * getViewMatrix(
            cameraAngles,
            cameraPos
        );

        auto now = chrono::high_resolution_clock::now();
        fseconds dt = chrono::duration_cast<fseconds>(now - past);
        wait -= dt;
        elapsed += dt;
        past = now;

        BindLock<Program> programLock(program);
        program.setUniform(U_GSAMPLER, 0);
        program.setUniform(U_AMBIENT, 0.4f);
        BindLock<Texture> texLock(tex);
        //program.setUniform(U_DIFF_DIRECTION, glm::vec3(0, 0, 1));
        for (MeshWrapper& mesh : meshes) {
            animate(mesh, dt);
            draw(mesh, U_WORLD, U_MVP, vpMatrix, program);
        }

        hudLines.draw();
        hudScore.draw();

        window.swap();

        if (wait > fseconds())
            continue;

        bool normalStep = false;
        fseconds levelPenalty(0.1f * (tetris.getStats().lines / 10));
        if (elapsed > delay - levelPenalty) {
            normalStep = true;
            tetris.collect();
            nextPiece |= tetris.step();
            elapsed = fseconds();
        } else {
            tetris.collect();
        }

        int leftPressed = 0, rightPressed = 0, upPressed = 0, downPressed = 0;
        if (!keysInit) {
            keys.onRepeat(GLFW_KEY_LEFT, fseconds(0.1f), [&]() {
                tetris.moveLeft();
            });
            keys.onRepeat(GLFW_KEY_RIGHT, fseconds(0.1f), [&]() {
                tetris.moveRight();
            });
            keys.onDown(GLFW_KEY_UP, [&]() {
                tetris.rotate();
            });
            keys.onRepeat(GLFW_KEY_DOWN, fseconds(0.03f), [&]() {
                if (!normalStep) {
                    tetris.collect();
                    nextPiece |= tetris.step();
                }
            });
            keys.onRepeat(GLFW_KEY_KP_4, fseconds(0.015f), [&]() {
                leftPressed = 1;
            });
            keys.onRepeat(GLFW_KEY_KP_6, fseconds(0.015f), [&]() {
                rightPressed = 1;
            });
            keys.onRepeat(GLFW_KEY_KP_8, fseconds(0.015f), [&]() {
                upPressed = 1;
            });
            keys.onRepeat(GLFW_KEY_KP_2, fseconds(0.015f), [&]() {
                downPressed = 1;
            });
            keysInit = true;
        }
        keys.advance(dt);
        if (nextPiece) {
            keys.stopRepeats(GLFW_KEY_DOWN);
            nextPiece = false;
        }
        cameraPos += glm::vec3 {
            -0.25 * leftPressed + 0.25 * rightPressed,
            0,
            -0.25 * upPressed + 0.25 * downPressed
        };
        bool mouseLeftPressed = window.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mouseLeftPressed) {
            glm::vec2 delta = window.getCursorPos() - cursor;
            cameraAngles += glm::vec3 { -0.5 * delta.y, -0.5 * delta.x, 0 };
        }
        cursor = window.getCursorPos();


        wait = copyState(tetris, &Tetris::getState, g_TetrisHor, g_TetrisVert, meshes[trunk].obj<Trunk>());
        copyState(tetris, &Tetris::getNextPieceState, 4, 4, meshes[nextPieceTrunk].obj<Trunk>());
    }
    return 0;
}
