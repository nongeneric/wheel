#include "Random.h"
#include "Window.h"
#include "Program.h"
#include "Tetris.h"
#include "Text.h"
#include "vformat.h"
#include "Config.h"

#include <cstring>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define GLM_FORCE_CXX11
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <boost/chrono.hpp>
#include <boost/log/trivial.hpp>

#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <map>

#define SHADER_VERSION "#version 330\n"

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;
const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

struct Vertex {
    glm::vec3 pos;
    glm::vec2 uv;
    glm::vec3 normal;
    Vertex(glm::vec3 pos, glm::vec2 uv, glm::vec3 normal = glm::vec3 {})
        : pos(pos), uv(uv), normal(normal) { }
};

class Mesh;
void setScale(Mesh& mesh, glm::vec3 scale);

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
        glActiveTexture(0);
    }
};

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

Texture oneColorTex(unsigned color) {
    Texture tex;
    char *c = reinterpret_cast<char*>(&color);
    std::swap(c[0], c[3]);
    std::swap(c[1], c[2]);
    tex.setImage(&color, 1, 1);
    return tex;
}

struct TrunkCube {
    Mesh mesh;
    CellInfo info;
    TrunkCube(Mesh& mesh, CellInfo info = CellInfo{}) : mesh(mesh), info(info) { }
};

const float cubeSpace = 0.2f;
class Trunk {
    std::vector<TrunkCube> _cubes;
    std::vector<TrunkCube> _border;
    glm::mat4 _pos;
    int _hor, _vert;
    bool _drawBorder;
    Texture _texBorder;
    std::vector<Texture> _pieceColors;
    TrunkCube& at(int x, int y) {
        return _cubes.at(_hor * y + x);
    }
public:
    Trunk(int hor, int vert, bool drawBorder)
        : _hor(hor),
          _vert(vert),
          _drawBorder(drawBorder),
          _texBorder(oneColorTex(0xFFFFFFFF))
    {
        _pieceColors.resize(PieceType::count);
        _pieceColors[PieceType::I] = oneColorTex(0xFF0000FF);
        _pieceColors[PieceType::J] = oneColorTex(0xFFFFFFFF);
        _pieceColors[PieceType::L] = oneColorTex(0xffa500FF);
        _pieceColors[PieceType::O] = oneColorTex(0xffff00FF);
        _pieceColors[PieceType::S] = oneColorTex(0xff00ffFF);
        _pieceColors[PieceType::T] = oneColorTex(0x00ffffFF);
        _pieceColors[PieceType::Z] = oneColorTex(0x00FF00FF);

        float xOffset = (1.0 * (hor + 2) + cubeSpace * (hor + 1) - 0.5) / -2.0f;
        for (int y = 0; y < vert; ++y) {
            for (int x = 0; x < hor; ++x) {
                Mesh cube = loadMesh();
                float xPos = xOffset + (cubeSpace + 1) * (x + 1);
                float yPos = y * (1 + cubeSpace) + cubeSpace;
                setPos(cube, glm::vec3 { xPos, yPos, 0 });
                _cubes.emplace_back(cube);
            }
        }
        if (!_drawBorder)
            return;
        for (int y = 0; y < vert; ++y) {
            Mesh left = loadMesh();
            setPos(left, glm::vec3 { xOffset, y * (1 + cubeSpace) + cubeSpace, 0 });
            _border.emplace_back(left);
            Mesh right = loadMesh();
            float xPos = xOffset + (hor + 1) * (1 + cubeSpace);
            float yPos = y * (1 + cubeSpace) + cubeSpace;
            setPos(right, glm::vec3 { xPos, yPos, 0 });
            _border.emplace_back(right);
        }
    }
    void animateDestroy(int x, int y, fseconds duration) {
        setAnimation(at(x, y).mesh, ScaleAnimation(duration, 1, 0));
    }
    void hide(int x, int y) {
        setScale(at(x, y).mesh, glm::vec3 {0, 0, 0});
    }
    void show(int x, int y) {
        setScale(at(x, y).mesh, glm::vec3 {1, 1, 1});
    }
    void setCellInfo(int x, int y, CellInfo& info) {
        at(x, y).info = info;
    }
    friend void animate(Trunk& trunk, fseconds dt);
    friend void draw(Trunk&, int, int, glm::mat4, Program&);
    friend void setPos(Trunk&, glm::vec3);
    friend glm::mat4 const& getPos(Trunk&);
};

void draw(Trunk& t, int mv_location, int mvp_location, glm::mat4 vp, Program& program) {
    for (TrunkCube& cube : t._cubes) {
        assert((unsigned)cube.info.piece < PieceType::count);
        BindLock<Texture> texLock(t._pieceColors.at(cube.info.piece));
        ::draw(cube.mesh, mv_location, mvp_location, vp * t._pos, program);
    }
    BindLock<Texture> texLock(t._texBorder);
    for (TrunkCube& cube : t._border)
        ::draw(cube.mesh, mv_location, mvp_location, vp * t._pos, program);
}

void setPos(Trunk& trunk, glm::vec3 pos) {
    trunk._pos = glm::translate({}, pos);
}

void animate(Trunk& trunk, fseconds dt) {
    for (TrunkCube& cube : trunk._cubes)
        animate(cube.mesh, dt);
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
        SHADER_VERSION
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
        SHADER_VERSION
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

enum { trunk, nextPieceTrunk };
std::vector<MeshWrapper> genMeshes() {
    std::vector<MeshWrapper> meshes {
        Trunk(g_TetrisHor, g_TetrisVert, true),
        Trunk(4, 4, false)
    };
    Trunk& tr = meshes[nextPieceTrunk].obj<Trunk>();
    setPos(tr, glm::vec3 { 12, 15, 0 } );
    return meshes;
}

fseconds copyState(
        Tetris& tetris,
        std::function<CellInfo(Tetris&, int x, int y)> getter,
        int xMax,
        int yMax,
        Trunk& trunk,
        fseconds duration)
{
    bool dying = false;
    for (int x = 0; x < xMax; ++x) {
        for (int y = 0; y < yMax; ++y) {
            CellInfo cellInfo = getter(tetris, x, y);
            assert((unsigned)cellInfo.piece < PieceType::count);
            trunk.setCellInfo(x, y, cellInfo);
            switch (cellInfo.state) {
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
        SHADER_VERSION
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
        SHADER_VERSION
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
        ButtonState(int state = GLFW_RELEASE, fseconds elapsed = fseconds{}, fseconds repeat = fseconds{})
            : state(state), elapsed(elapsed), repeat(repeat) { }
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

void drawText(std::string str, Text& text, HudElem& elem, unsigned x, unsigned y, glm::vec2 framebuffer) {
    auto bitmap = text.renderText(str, framebuffer.y * 0.05);
    unsigned height = FreeImage_GetHeight(bitmap.get());
    elem.setBitmap(
        FreeImage_GetBits(bitmap.get()),
        FreeImage_GetWidth(bitmap.get()),
        height,
        x, y, framebuffer.x, framebuffer.y
    );
}

class HudList {
    struct ListItem {
        std::string prevText;
        std::string text;
        bool invalidated = false;
        HudElem elem;
    };
    glm::vec2 _prevFrame;
    std::vector<ListItem> _lines;
    Text* _text;
public:
    HudList(int lines, Text* text) : _lines(lines), _text(text) { }
    void setLine(int i, std::string text) {
        ListItem& item = _lines.at(i);
        if (item.text != text) {
            item.prevText = item.text;
            item.text = text;
            item.invalidated = true;
        }
    }
    void draw(glm::vec2 frame) {
        if (_prevFrame != frame) {
            for (ListItem& item : _lines)
                item.invalidated = true;
            _prevFrame = frame;
        }

        int i = 1;
        for (ListItem& item : _lines) {
            if (item.invalidated) {
                auto bitmap = _text->renderText(item.text, frame.y * 0.05);
                unsigned height = FreeImage_GetHeight(bitmap.get());
                item.elem.setBitmap(
                    FreeImage_GetBits(bitmap.get()),
                    FreeImage_GetWidth(bitmap.get()),
                    height,
                    0,
                    frame.y - (height + 0.01 * frame.y) * i, frame.x, frame.y
                );
                item.invalidated = false;
            }
            i++;
            item.elem.draw();
        }
    }
};

float speedCurve(int level) {
    if (level <= 15)
        return 0.05333f * level;
    return 0.6125f + 0.0125f * level;
}

class Generator {
    Random<unsigned> _random;
public:
    Generator() : _random(0, PieceType::count - 1) { }
    PieceType::t operator()() {
        return static_cast<PieceType::t>(_random());
    }
};

class FpsCounter {
    fseconds _elapsed;
    int _framesCount = 0;
    int _prevFPS = -1;
public:
    void advance(fseconds dt) {
        _elapsed += dt;
        _framesCount++;
        if (_elapsed > fseconds(1.0f)) {
            _prevFPS = _framesCount;
            _framesCount = 0;
            _elapsed = fseconds();
        }
    }
    int fps() const {
        return _prevFPS;
    }
};

class Camera {
    glm::vec3 _defaultAngles{0, 0, 0};
    glm::vec3 _defaultPos{0, 13, 50};
    glm::vec3 _angles = _defaultAngles;
    glm::vec3 _pos = _defaultPos;
    glm::vec2 _cursor{ 300, 300 };
public:
    glm::mat4 view() {
        glm::mat4 rotation = glm::rotate( glm::mat4(), -_angles.x, glm::vec3 { 1, 0, 0 } ) *
                             glm::rotate( glm::mat4(), -_angles.y, glm::vec3 { 0, 1, 0 } ) *
                             glm::rotate( glm::mat4(), -_angles.z, glm::vec3 { 0, 0, 1 } );
        glm::mat4 translation = glm::translate( {}, -_pos );
        return translation * rotation;
    }
    void updateKeyboard(bool left, bool right, bool up, bool down) {
        _pos += glm::vec3 {
            -0.25 * left + 0.25 * right,
            0,
            -0.25 * up + 0.25 * down
        };
    }
    void updateMouse(Window& window) {
        bool mouseLeftPressed = window.getMouseButton(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (mouseLeftPressed) {
            glm::vec2 delta = window.getCursorPos() - _cursor;
            _angles += glm::vec3 { -0.5 * delta.y, -0.5 * delta.x, 0 };
        }
        _cursor = window.getCursorPos();
    }
    void reset() {
        _angles = _defaultAngles;
        _pos = _defaultPos;
    }
};

bool loadConfig(TetrisConfig& config) {
    try {
        config.load("config.xml");
    } catch (...) {
        BOOST_LOG_TRIVIAL(error) << "an error ocurred while loading the config file";
        return false;
    }
    return true;
}

int desktop_entry() {
    TetrisConfig config;
    if (!loadConfig(config)) {
        return 1;
    }

    Window window("wheel", config.fullScreen, config.screenWidth, config.screenHeight);
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

    Tetris tetris(g_TetrisHor, g_TetrisVert, Generator(), config.initialLevel);
    Camera camera;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0,0);
    auto past = chrono::high_resolution_clock::now();
    fseconds elapsed;
    fseconds wait;

    Text text;
    HudElem hudGameOver;

    HudList hudList(6, &text);
    std::string gameOverText = "Game Over!";
    std::string pauseText = "PAUSED";
    fseconds delay = fseconds(1.0f);

    FpsCounter fps;
    Keyboard keys(&window);
    bool keysInit = false;
    bool nextPiece = false;
    bool paused = false;
    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::vec2 size = window.getFramebufferSize();
        glViewport(0, 0, size.x, size.y);

        hudList.setLine(0, vformat("Lines: %d", tetris.getStats().lines));
        hudList.setLine(1, vformat("Score: %d", tetris.getStats().score));
        hudList.setLine(2, vformat("Level: %d", tetris.getStats().level));
        if (config.showFps) {
            hudList.setLine(3, vformat("FPS: %d", fps.fps()));
        }
        hudList.setLine(5, paused ? pauseText : "");

        float aspect = size.x / size.y;
        glm::mat4 proj;
        if (config.orthographic) {
            proj = glm::ortho(
                 -13.0f * aspect, 13.0f * aspect,
                 -13.0f, 13.0f, 1.0f, 1000.0f);
        } else {
            proj = glm::perspective(30.0f, size.x / size.y, 1.0f, 1000.0f);
        }
        glm::mat4 vpMatrix = proj * camera.view();

        auto now = chrono::high_resolution_clock::now();
        fseconds dt = chrono::duration_cast<fseconds>(now - past);
        fps.advance(dt);
        if (paused)
            dt = fseconds();
        wait -= dt;
        elapsed += dt;
        past = now;

        bool waiting = wait > fseconds();

        bool canManuallyMove = !tetris.getStats().gameOver && !waiting && !paused;
        bool normalStep = false;
        fseconds levelPenalty(speedCurve(tetris.getStats().level));
        if (elapsed > delay - levelPenalty && canManuallyMove) {
            normalStep = true;
            tetris.collect();
            nextPiece |= tetris.step();
            elapsed = fseconds();
        }

        int leftPressed = 0, rightPressed = 0, upPressed = 0, downPressed = 0;
        if (!keysInit) {
            keys.onRepeat(GLFW_KEY_LEFT, fseconds(0.1f), [&]() {
                if (canManuallyMove) {
                    tetris.moveLeft();
                }
            });
            keys.onRepeat(GLFW_KEY_RIGHT, fseconds(0.1f), [&]() {
                if (canManuallyMove) {
                    tetris.moveRight();
                }
            });
            keys.onDown(GLFW_KEY_UP, [&]() {
                if (canManuallyMove) {
                    tetris.rotate();
                }
            });
            keys.onRepeat(GLFW_KEY_DOWN, fseconds(0.03f), [&]() {
                if (!normalStep && canManuallyMove) {
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
            keys.onDown(GLFW_KEY_KP_5, [&]() {
                camera.reset();
            });
            keys.onDown(GLFW_KEY_ESCAPE, [&]() {
                wait = fseconds();
                hudList.setLine(4, "");
                tetris.reset();
            });
            keys.onDown(GLFW_KEY_PAUSE, [&]() {
                paused = !paused;
            });
            keysInit = true;
        }
        keys.advance(dt);

        if (nextPiece) {
            keys.stopRepeats(GLFW_KEY_DOWN);
            nextPiece = false;
        }
        camera.updateKeyboard(leftPressed, rightPressed, upPressed, downPressed);
        camera.updateMouse(window);

        if (!waiting) {
            wait = copyState(tetris,
                             &Tetris::getState,
                             g_TetrisHor,
                             g_TetrisVert,
                             meshes[trunk].obj<Trunk>(),
                             fseconds(1.0f) - levelPenalty);
            copyState(tetris, &Tetris::getNextPieceState, 4, 4, meshes[nextPieceTrunk].obj<Trunk>(), fseconds());
        }
        tetris.collect();

        BindLock<Program> programLock(program);
        program.setUniform(U_GSAMPLER, 0);
        program.setUniform(U_AMBIENT, 0.4f);
        //program.setUniform(U_DIFF_DIRECTION, glm::vec3(0, 0, 1));
        for (MeshWrapper& mesh : meshes) {
            animate(mesh, dt);
            draw(mesh, U_WORLD, U_MVP, vpMatrix, program);
        }

        if (tetris.getStats().gameOver) {
            hudList.setLine(4, gameOverText);
        }
        hudList.draw(size);

        window.swap();
    }
    return 0;
}
