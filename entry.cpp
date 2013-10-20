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

#include "rstd.h"
#include <functional>
#include <stdexcept>
#include <string>
#include <vector>
#include <memory>
#include <assert.h>
#include <map>
#include <stack>

#define SHADER_VERSION "#version 140\n"

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
        std::cout << importer.GetErrorString();
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
        "uniform mat4 mvp;"
        "uniform mat4 world;"
        "in vec4 position;" // layout(location = 0)
        "in vec2 texCoord;" // layout(location = 1)
        "in vec3 normal;" // layout(location = 2)
        "out vec2 f_texCoord;"
        "out vec3 f_normal;"
        "void main() {"
        "    gl_Position = mvp * position;"
        "    f_texCoord = texCoord;"
        "    f_normal = (world * vec4(normal, 0.0)).xyz;"
        "}"
        ;

std::string fragmentShader =
        SHADER_VERSION
        "in vec2 f_texCoord;"
        "in vec3 f_normal;"
        "out vec4 outputColor;"
        "uniform sampler2D gSampler;"
        "uniform float ambient;"
        // white color
        "vec4 getDiffuseFactor(vec3 direction, vec4 baseColor) {"
        "   float factor = dot(normalize(f_normal), -direction);"
        "   vec4 color;"
        "   if (factor > 0) {"
        "       color = baseColor * factor;"
        "   } else {"
        "       color = vec4(0.0, 0.0, 0.0, 0.0);"
        "   }"
        "   return color;"
        "}"
        "void main() {"
        "   vec4 source1 = getDiffuseFactor(vec3(-1.0, 0.0, -1.0), vec4(1,1,1,0.6));"
        "   vec4 source2 = getDiffuseFactor(vec3(1.0, 0.0, 1.0), vec4(1,0,0,0.4));"
        "   outputColor = texture2D(gSampler, f_texCoord.st) * (ambient + source1 + source2);"
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
        "   outputColor = vec4(color,texture2D(sampler, f_uv).a);"
        "}"
    );
    res.link();
    return res;
}

VertexBuffer createFullScreenVertices() {
    return VertexBuffer {{
        {glm::vec3 { -1, -1, 0 }, glm::vec2 { 0, 0 }},
        {glm::vec3 { 1, -1, 0 }, glm::vec2 { 1, 0 }},
        {glm::vec3 { -1, 1, 0.}, glm::vec2 { 0, 1 }},
        {glm::vec3 { 1, 1, 0 }, glm::vec2 { 1, 1 }}
    }};
}

IndexBuffer createFullScreenIndices() {
    return IndexBuffer{{
        0, 1, 3, 0, 2, 3
    }};
}

class CrispBitmap {
    Texture _tex;
    VAO _vao;
    VertexBuffer _vertices;
    IndexBuffer _indices;
    Program _program;
    GLuint _sampler;
    GLuint _transformUniform;
    GLuint _colorUniform;
    glm::mat4 _transform;
    long _width;
    long _height;
    glm::vec2 _framebuffer;
    glm::vec3 _color;
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
    CrispBitmap() : _vertices(createFullScreenVertices()),
                _indices(createFullScreenIndices()),
                _program(createBitmapProgram())
    {
        _sampler = _program.getUniformLocation("sampler");
        _transformUniform = _program.getUniformLocation("transform");
        _colorUniform = _program.getUniformLocation("color");
        _color = glm::vec3 { 1.0f, 1.0f, 1.0f};
        initVao();
    }

    void setBitmap(FIBITMAP* bitmap, glm::vec2 framebuffer) {
        _framebuffer = framebuffer;
        _width = FreeImage_GetWidth(bitmap);
        _height = FreeImage_GetHeight(bitmap);
        _tex.setImage(FreeImage_GetBits(bitmap), _width, _height);
    }

    void setPos(float x, float y) {
        float normWidth = (float)_width / _framebuffer.x;
        float normHeight = (float)_height / _framebuffer.y;
        auto scale = glm::scale( {}, glm::vec3 {normWidth, normHeight, 1});
        float normDx = x / _framebuffer.x * 2;
        float normDy = y / _framebuffer.y * 2;
        auto translate = glm::translate( {}, glm::vec3 {normDx + normWidth - 1, normDy + normHeight - 1, 0});
        _transform = translate * scale;
    }

    void draw() {
        BindLock<Program> programLock(_program);
        _program.setUniform(_sampler, 0);
        _program.setUniform(_transformUniform, _transform);
        _program.setUniform(_colorUniform, _color);
        BindLock<Texture> texLock(_tex);
        BindLock<VAO> vaoLock(_vao);
        glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, 0);
    }

    void setColor(glm::vec3 color) {
        _color = color;
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

class TextLine {
    Text* _text;
    CrispBitmap _elem;
    std::string _line;
    float _height;
    glm::vec2 _framebuffer;
    bool _lineChanged = false;
    bool _framebufferChanged = false;
    float _posX;
    float _posY;
    BitmapPtr _bitmap;

    void redrawText() {
        if (_lineChanged || _framebufferChanged) {
            _lineChanged = false;
            _framebufferChanged = false;
            _bitmap = _text->renderText(_line, _height * _framebuffer.y);
            _elem.setBitmap(_bitmap.get(), _framebuffer);
            _elem.setPos(_posX, _posY);
        }
    }

public:
    TextLine(Text* text, float height) {
        _height = height;
        _text = text;
    }

    void setPos(float x, float y) {
        _posX = x;
        _posY = y;
        _elem.setPos(x, y);
    }

    void setFramebuffer(glm::vec2 framebuffer) {
        if (_framebuffer == framebuffer)
            return;
        _framebufferChanged = true;
        _framebuffer = framebuffer;
    }

    void set(std::string line) {
        if (line == _line)
            return;
        _lineChanged = true;
        _line = line;
    }

    glm::vec2 size() {
        redrawText();
        return glm::vec2 {
            FreeImage_GetWidth(_bitmap.get()),
            FreeImage_GetHeight(_bitmap.get())
        };
    }

    void draw() {
        redrawText();
        _elem.draw();
    }

    void setColor(glm::vec3 color) {
        _elem.setColor(color);
    }
};

class HudList {
    std::vector<TextLine> _lines;
    Text* _text;
    glm::vec2 _framebuffer;
public:
    HudList(int lines, Text* text) : _text(text) {
        for (int i = 0; i < lines; ++i) {
            _lines.emplace_back(text, 0.05);
        }
    }

    void setLine(int i, std::string text) {
        _lines[i].set(text);
        _lines[i].setPos(0, _framebuffer.y * (1.0f - (i + 1) * 0.07));
    }

    void setFramebuffer(glm::vec2 framebuffer) {
        _framebuffer = framebuffer;
        for (auto& line : _lines)
            line.setFramebuffer(framebuffer);
    }

    void draw() {
        for (TextLine& line : _lines) {
            line.draw();
        }
    }
};

float speedCurve(int level) {
    if (level <= 15)
        return 0.05333f * level;
    return 0.65f + 0.01f * level;
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
        std::cout << "an error ocurred while loading the config file";
        return false;
    }
    return true;
}

struct MainProgramInfo {
    Program program;
    GLuint U_MVP;
    GLuint U_WORLD;
    GLuint U_GSAMPLER;
    GLuint U_AMBIENT;
};

MainProgramInfo createMainProgram() {
    MainProgramInfo info;
    Program& program = info.program;
    program.addVertexShader(vertexShader);
    program.addFragmentShader(fragmentShader);
    program.link();
    info.U_MVP = program.getUniformLocation("mvp");
    info.U_WORLD = program.getUniformLocation("world");
    info.U_GSAMPLER = program.getUniformLocation("gSampler");
    info.U_AMBIENT = program.getUniformLocation("ambient");
    //const GLuint U_DIFF_DIRECTION = program.getUniformLocation("diffDirection");
    return info;
}

glm::mat4 getProjection(glm::vec2 framebuffer, bool orthographic) {
    glViewport(0, 0, framebuffer.x, framebuffer.y);
    float aspect = framebuffer.x / framebuffer.y;
    if (orthographic) {
        return glm::ortho(
             -13.0f * aspect, 13.0f * aspect,
             -13.0f, 13.0f, 1.0f, 1000.0f);
    } else {
        return glm::perspective(30.0f, framebuffer.x / framebuffer.y, 1.0f, 1000.0f);
    }
}

class CameraController {
    Camera* _camera;
    Keyboard _keys;
    Window* _window;
    bool _leftPressed;
    bool _rightPressed;
    bool _upPressed;
    bool _downPressed;
public:
    CameraController(Window* window, Camera *camera)
        : _camera(camera), _keys(window), _window(window)
    {
        _keys.onRepeat(GLFW_KEY_KP_4, fseconds(0.015f), [&]() {
            _leftPressed = true;
        });
        _keys.onRepeat(GLFW_KEY_KP_6, fseconds(0.015f), [&]() {
            _rightPressed = true;
        });
        _keys.onRepeat(GLFW_KEY_KP_8, fseconds(0.015f), [&]() {
            _upPressed = true;
        });
        _keys.onRepeat(GLFW_KEY_KP_2, fseconds(0.015f), [&]() {
            _downPressed = true;
        });
        _keys.onDown(GLFW_KEY_KP_5, [&]() {
            _camera->reset();
        });
    }
    void advance(fseconds dt) {
        _leftPressed = false;
        _rightPressed = false;
        _upPressed = false;
        _downPressed = false;
        _keys.advance(dt);
        _camera->updateKeyboard(_leftPressed, _rightPressed, _upPressed, _downPressed);
        _camera->updateMouse(*_window);
    }
};

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
    res.link();
    return res;
}

class Painter2D {
    Program _program;
    VAO _vao;
    VertexBuffer _vertices;
    IndexBuffer _indices;
    std::vector<std::tuple<glm::vec2, glm::vec2, glm::vec4>> _rects;
    GLuint _uTransform;
    GLuint _uColor;
    void initVao() {
        BindLock<VAO> lock(_vao);
        _vertices.bind();
        _indices.bind();
        glEnableVertexAttribArray(0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12); // not used
    }
public:
    Painter2D()
        : _program{createPainterProgram()},
          _vertices{createFullScreenVertices()},
          _indices{createFullScreenIndices()}
    {
        initVao();
        _uTransform = _program.getUniformLocation("transform");
        _uColor = _program.getUniformLocation("color");
    }
    void rect(glm::vec2 pos, glm::vec2 size, glm::vec4 color) {
        _rects.emplace_back(pos * 2.0f, size, color);
    }
    void draw() {
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
};

template<typename T, typename R>
std::vector<R> fmap(std::vector<T> const& a, std::function<R(const T&)> f) {
    std::vector<R> res;
    for (const T& t : a) {
        res.push_back(f(t));
    }
    return res;
}

class MenuLeaf {
    std::vector<std::string> _values;
    std::string _value;
    TextLine _line;    
    std::string _lineText;
public:
    MenuLeaf(Text* textCache, std::vector<std::string> values, std::string line)
        : _values(values), _line(textCache, 0.06f), _lineText(line)
    {
        _line.set(line);
    }
    void setFramebuffer(glm::vec2 framebuffer) {
        _line.setFramebuffer(framebuffer);
    }
    void setPos(glm::vec2 pos) {
        _line.setPos(pos.x, pos.y);
    }
    glm::vec2 size() {
        return _line.size();
    }
    void draw() {
        _line.draw();
    }
    void setValue(std::string value) {
        _value = value;
        _line.set(_lineText + ": " + value);
    }

    std::vector<std::string> const& values() const {
        return _values;
    }

    std::string value() const {
        return _value;
    }

    void highlight(bool on) {
        if (on) {
            _line.setColor(glm::vec3 { 1.0f, 1.0f, 1.0f});
        } else {
            _line.setColor(glm::vec3 { 0.7f, 0.7f, 0.7f});
        }
    }
};

class Menu {
    struct LeafPos {
        glm::vec2 left;
        glm::vec2 center;
        glm::vec2 right;
    };

    static constexpr float _lineHeight = 1.2f;

    std::vector<std::unique_ptr<MenuLeaf>> _leafs;
    std::string _text;
    Text* _textCache;
    glm::vec2 _pos;
    Menu* _parent = nullptr;
    std::vector<LeafPos> _leafPositions;
    glm::vec2 _framebuffer;
    bool _animating = false;
    fseconds _elapsed;
    bool _isAssembling;
    glm::vec2 _size;

    float width() {
        float width = 0;
        for (auto const& leaf : _leafs) {
            width = std::max(width, leaf->size().x);
        }
        return width;
    }

    void calcPositions() {
        _leafPositions.clear();
        float x = _pos.x;
        float y = _pos.y;
        float width = this->width();
        for (auto it = _leafs.rbegin(); it != _leafs.rend(); ++it) {
            auto size = (*it)->size();
            glm::vec2 center { x + (width - size.x) / 2, y };
            glm::vec2 left { 0, y };
            glm::vec2 right { (_framebuffer - size).x, y};
            _leafPositions.insert(begin(_leafPositions), {left, center, right});            
            y += size.y * _lineHeight;
        }
        _size = glm::vec2 { width, y - _pos.y };
    }

public:
    Menu(Text* textCache) : _textCache(textCache) { }

    void animate(bool assemble) {
        _animating = assemble;
        _elapsed = fseconds();
        _isAssembling = assemble;
    }

    MenuLeaf* addLeaf(std::string text,
                      std::vector<std::string> values)
    {
        _leafs.emplace_back(new MenuLeaf(_textCache, values, text));
        return _leafs.back().get();
    }

    std::vector<MenuLeaf*> leafs() {
        return fmap<std::unique_ptr<MenuLeaf>, MenuLeaf*>(_leafs, [](const std::unique_ptr<MenuLeaf>& ptr) { return ptr.get(); });
    }

    Menu* parent() {
        return _parent;
    }

    void setPos(glm::vec2 pos) {
        _pos = pos;
        calcPositions();        
    }

    void setFramebuffer(glm::vec2 framebuffer) {
        _framebuffer = framebuffer;
        for (auto const& leaf : _leafs) {
            leaf->setFramebuffer(framebuffer);
        }
    }

    glm::vec2 size() {
        return _size;
    }

    float animCurve(float a, float b, float ratio) {
        return a + ratio * ratio * ratio * ratio * (b - a);
    }

    void advance(fseconds dt) {        
        if (!_animating)
            return;
        fseconds duration(0.4f);
        if (_elapsed > duration) {
            _animating = false;
            return;
        }        
        _elapsed += dt;
        float ratio = std::min(_elapsed.count() / duration.count(), 1.0f);
        ratio = _isAssembling ? 1.0f - ratio : ratio;
        for (unsigned i = 0; i < _leafs.size(); ++i) {
            if (i & 1) {
                _leafs[i]->setPos( glm::vec2 {
                    animCurve(_leafPositions[i].center.x, _leafPositions[i].right.x, ratio),
                    _leafPositions[i].center.y });
            } else {
                _leafs[i]->setPos( glm::vec2 {
                    animCurve(_leafPositions[i].center.x, _leafPositions[i].left.x, ratio),
                    _leafPositions[i].center.y });
            }
        }
    }

    void draw() {
        for (auto const& leaf : _leafs) {
            leaf->draw();
        }
    }
};

class MenuController {
    std::map<MenuLeaf*, std::function<void()>> _handlers;
    Menu* _menu;
    MenuLeaf *_leaf;
    Keyboard _keys;
    std::stack<Menu*> _history;

    void advanceFocus(int delta) {
        _leaf->highlight(false);
        auto leafs = _menu->leafs();
        auto it = std::find(begin(leafs), end(leafs), _leaf);
        assert(it != end(leafs));
        unsigned index = std::distance(begin(leafs), it);
        index = (index + leafs.size() + delta) % leafs.size();
        _leaf = leafs[index];
        _leaf->highlight(true);
    }

    void clearHighlights() {
        for (auto& leaf : _menu->leafs()) {
            leaf->highlight(false);
        }
        _leaf->highlight(true);
    }

    void advanceValue(int delta, MenuLeaf* leaf) {
        auto const& values = leaf->values();
        auto it = std::find(begin(values), end(values), _leaf->value());
        assert(it != end(values));
        unsigned index = std::distance(begin(values), it);
        index = (index + values.size() + delta) % values.size();
        leaf->setValue(values[index]);
        assert(_handlers.find(_leaf) != end(_handlers));
        _handlers[_leaf]();
    }

public:
    MenuController(Menu* menu, Window* window)
        : _menu(menu), _keys(window)
    {        
        setActiveMenu(menu);
        clearHighlights();
        _keys.onDown(GLFW_KEY_DOWN, [&]() {
            advanceFocus(1);
        });
        _keys.onDown(GLFW_KEY_UP, [&]() {
            advanceFocus(-1);
        });
        _keys.onDown(GLFW_KEY_ENTER, [&]() {
            if (_leaf->values().empty()) {
                assert(_handlers.find(_leaf) != end(_handlers));
                _handlers[_leaf]();
            }
        });
        _keys.onDown(GLFW_KEY_ESCAPE, [&]() {
            if (_history.size() > 0) {
                setActiveMenu(_history.top(), false);
                _history.pop();
            } else {
                _handlers[nullptr]();
            }
        });
        _keys.onRepeat(GLFW_KEY_LEFT, fseconds(0.15f), [&]() {
            if (_leaf->values().empty())
                return;
            advanceValue(-1, _leaf);
        });
        _keys.onRepeat(GLFW_KEY_RIGHT, fseconds(0.15f), [&]() {
            if (_leaf->values().empty())
                return;
            advanceValue(1, _leaf);
        });
    }

    void onValueChanged(MenuLeaf* leaf, std::function<void()> handler) {
        _handlers[leaf] = handler;
    }

    void advance(fseconds dt) {
        _keys.advance(dt);
        _menu->advance(dt);
    }

    void setActiveMenu(Menu* menu, bool saveState = true) {
        if (saveState)
            _history.push(_menu);
        _menu = menu;
        _leaf = menu->leafs().front();
        clearHighlights();
        _menu->animate(true);
    }

    void draw() {
        _menu->draw();
    }

    void show(bool on) {
        clearHighlights();
        _leaf = _menu->leafs().front();
        _menu->animate(on);
        advance(fseconds());
    }
};

struct MainMenuStructure {
    MenuLeaf* resume;
    MenuLeaf* restart;
    MenuLeaf* options;
    MenuLeaf* hallOfFame;
    MenuLeaf* exit;
};

MainMenuStructure initMainMenu(Menu& menu) {
    MainMenuStructure res;
    res.resume = menu.addLeaf("Resume", {});
    res.restart = menu.addLeaf("Restart", {});
    res.options = menu.addLeaf("Options", {});
    res.hallOfFame = menu.addLeaf("Hall of fame", {});
    res.exit = menu.addLeaf("Exit", {});
    return res;
}

struct OptionsMenuStructure {
    MenuLeaf* initialSpeed;
    MenuLeaf* fullscreen;
    MenuLeaf* resolution;
};

std::vector<std::string> genNumbers(int count) {
    std::vector<std::string> res;
    for (int i = 0; i < count; ++i)
        res.push_back(vformat("%d", i));
    return res;
}

OptionsMenuStructure initOptionsMenu(Menu& menu, TetrisConfig config) {
    OptionsMenuStructure res;
    int speed = std::min(config.initialLevel, 19u);
    (res.initialSpeed = menu.addLeaf("Initial speed", genNumbers(20)))->setValue(vformat("%d", speed));
    (res.fullscreen = menu.addLeaf("Fullscreen", {"On", "Off"}))->setValue(config.fullScreen ? "On" : "Off");
    (res.resolution = menu.addLeaf("Resolution", {"1920x1080"}))->setValue("1920x1080");
    return res;
}

int desktop_entry() {
    TetrisConfig config;
    if (!loadConfig(config)) {
        return 1;
    }

    Window window("wheel", config.fullScreen, config.screenWidth, config.screenHeight);
    MainProgramInfo program = createMainProgram();
    std::vector<MeshWrapper> meshes = genMeshes();
    Tetris tetris(g_TetrisHor, g_TetrisVert, Generator(), config.initialLevel);
    Camera camera;
    CameraController camController(&window, &camera);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0,0);
    auto past = chrono::high_resolution_clock::now();
    fseconds elapsed;
    fseconds wait;

    Text text;
    CrispBitmap hudGameOver;

    HudList hudList(5, &text);
    std::string gameOverText = "Game Over!";
    std::string pauseText = "PAUSED";
    fseconds delay = fseconds(1.0f);

    Painter2D p2d;
    p2d.rect(glm::vec2 { 0, 0 }, glm::vec2 { 1.0f, 1.0f }, glm::vec4 {0, 0, 0, 0.6});
    //p2d.rect(glm::vec2 { 0, 0.45f }, glm::vec2 { 0.3f, 0.1f }, glm::vec4 {0.5, 0.5, 0.5, 0.5});

    Menu mainMenu(&text), optionsMenu(&text);
    MainMenuStructure mainMenuStructure = initMainMenu(mainMenu);
    OptionsMenuStructure optionsMenuStructure = initOptionsMenu(optionsMenu, config);

    FpsCounter fps;
    Keyboard keys(&window);
    bool canManuallyMove;
    bool normalStep;
    bool nextPiece = false;
    bool paused = false;
    bool exit = false;
    MenuController menu(&mainMenu, &window);
    keys.onRepeat(GLFW_KEY_LEFT, fseconds(0.07f), [&]() {
        if (canManuallyMove) {
            tetris.moveLeft();
        }
    });
    keys.onRepeat(GLFW_KEY_RIGHT, fseconds(0.07f), [&]() {
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
    keys.onDown(GLFW_KEY_ESCAPE, [&]() {
        paused = true;
        menu.show(true);
    });

    menu.onValueChanged(mainMenuStructure.resume, [&]() {
        paused = false;
    });
    menu.onValueChanged(mainMenuStructure.restart, [&]() {
        wait = fseconds();
        hudList.setLine(4, "");
        tetris.reset();
        paused = false;
    });
    menu.onValueChanged(mainMenuStructure.options, [&]() {
        menu.setActiveMenu(&optionsMenu);
    });
    menu.onValueChanged(mainMenuStructure.exit, [&]() {
        exit = true;
    });
    menu.onValueChanged(nullptr, [&]() {
        paused = false;
    });
    auto empty = [](){};
    menu.onValueChanged(optionsMenuStructure.fullscreen, empty);
    menu.onValueChanged(optionsMenuStructure.initialSpeed, empty);
    menu.onValueChanged(optionsMenuStructure.resolution, empty);

    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hudList.setLine(0, vformat("Lines: %d", tetris.getStats().lines));
        hudList.setLine(1, vformat("Score: %d", tetris.getStats().score));
        hudList.setLine(2, vformat(u8"Уровень: %d", tetris.getStats().level));
        if (config.showFps) {
            hudList.setLine(3, vformat("FPS: %d", fps.fps()));
        }

        glm::vec2 framebuffer = window.getFramebufferSize();
        glm::mat4 proj = getProjection(framebuffer, config.orthographic);
        glm::mat4 vpMatrix = proj * camera.view();

        hudList.setFramebuffer(framebuffer);
        mainMenu.setFramebuffer(framebuffer);
        mainMenu.setPos((framebuffer - mainMenu.size()) * 0.5f);
        optionsMenu.setFramebuffer(framebuffer);
        optionsMenu.setPos((framebuffer - optionsMenu.size()) * 0.5f);

        auto now = chrono::high_resolution_clock::now();
        fseconds dt = chrono::duration_cast<fseconds>(now - past);
        fps.advance(dt);
        fseconds realDt = dt;
        if (paused)
            dt = fseconds();
        wait -= dt;
        elapsed += dt;
        past = now;

        bool waiting = wait > fseconds();

        canManuallyMove = !tetris.getStats().gameOver && !waiting && !paused;
        normalStep = false;
        fseconds levelPenalty(speedCurve(tetris.getStats().level));
        if (elapsed > delay - levelPenalty && canManuallyMove) {
            normalStep = true;
            tetris.collect();
            nextPiece |= tetris.step();
            elapsed = fseconds();
        }

        if (paused) {
            menu.advance(realDt);
        } else {
            keys.advance(dt);
        }
        camController.advance(dt);

        if (nextPiece) {
            keys.stopRepeats(GLFW_KEY_DOWN);
            nextPiece = false;
        }        

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

        BindLock<Program> programLock(program.program);
        program.program.setUniform(program.U_GSAMPLER, 0);
        program.program.setUniform(program.U_AMBIENT, 0.4f);
        //program.setUniform(U_DIFF_DIRECTION, glm::vec3(0, 0, 1));
        for (MeshWrapper& mesh : meshes) {
            animate(mesh, dt);
            draw(mesh, program.U_WORLD, program.U_MVP, vpMatrix, program.program);
        }

        if (tetris.getStats().gameOver) {
            hudList.setLine(4, gameOverText);
        }        

        glDisable(GL_DEPTH_TEST);
        hudList.draw();
        if (paused) {
            p2d.draw();
            menu.draw();
        }
        glEnable(GL_DEPTH_TEST);

        window.swap();

        if (exit)
            return 0;
    }
    return 0;
}
