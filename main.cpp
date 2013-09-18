#include "Random.h"
#include "Window.h"
#include "Program.h"
#include "Tetris.h"

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

namespace chrono = boost::chrono;
using fseconds = chrono::duration<float>;
const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

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
    const aiScene* scene = importer.ReadFile("/home/tr/Desktop/cube.ply",
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

class Trunk {
    std::vector<Mesh> _cubes;
    std::vector<Mesh> _border;
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
                Mesh cube = loadMesh();
                setPos(cube, glm::vec3 {x * 1.3, y * 1.3 + 0.3, 0});
                _cubes.push_back(cube);
            }
        }
        for (int y = 0; y < vert; ++y) {
            Mesh left = loadMesh();
            setPos(left, glm::vec3 { -1.3, y * 1.3 + 0.3, 0 });
            _border.push_back(left);
            Mesh right = loadMesh();
            setPos(right, glm::vec3 { hor * 1.3, y * 1.3 + 0.3, 0 });
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
        "   vec4 source1 = getDiffuseFactor(vec3(-1.0, 0.0, -1.0), vec4(1,1,1,1));\n"
        "   vec4 source2 = getDiffuseFactor(vec3(1.0, 0.0, 1.0), vec4(1,0,0,1));\n"
        "   outputColor = texture2D(gSampler, f_texCoord.st) * (ambient + source1 + source2);\n"
        "}"
        ;

enum { red_plato, green_plato, blue_plato, gray_plato, trunk };
std::vector<MeshWrapper> genMeshes() {
    GLfloat plato_size = 20.0f;
    std::vector<MeshWrapper> meshes {
        genZXPlato(0.0f, plato_size),
        genZXPlato(1.0f, plato_size),
        genZXPlato(0.0f, plato_size),
        genZXPlato(1.0f, plato_size),
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
    const GLuint U_WORLD = program.getUniformLocation("world");
    const GLuint U_GSAMPLER = program.getUniformLocation("gSampler");
    const GLuint U_AMBIENT = program.getUniformLocation("ambient");
    //const GLuint U_DIFF_DIRECTION = program.getUniformLocation("diffDirection");

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
    int prevKP2State = GLFW_RELEASE;

    fseconds wait;

    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    GLuint color[] = { 0xFFFFFFFF, 0xFF000000 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, color);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glActiveTexture(tex);
    bool nextPiece = false;
    bool fastFall = false;
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
        program.setUniform(U_GSAMPLER, 0);
        program.setUniform(U_AMBIENT, 0.4f);
        //program.setUniform(U_DIFF_DIRECTION, glm::vec3(0, 0, 1));
        for (MeshWrapper& mesh : meshes) {
            animate(mesh, dt);
            draw(mesh, U_WORLD, U_MVP, vpMatrix, program);
        }

        window.swap();

        if (wait > fseconds())
            continue;

        if (elapsed > fseconds(0.4f)) {
            nextPiece |= tetris.step();
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
        if (window.getKey(GLFW_KEY_KP_3) == GLFW_PRESS && prevKP3State == GLFW_RELEASE) {
            tetris.moveRight();
        }
        if (window.getKey(GLFW_KEY_KP_1) == GLFW_PRESS && prevKP1State == GLFW_RELEASE) {
            tetris.moveLeft();
        }
        if (window.getKey(GLFW_KEY_KP_6) == GLFW_PRESS && prevKP6State == GLFW_RELEASE) {
            tetris.rotate();
        }        
        if (window.getKey(GLFW_KEY_KP_2) == GLFW_PRESS && prevKP2State == GLFW_RELEASE) {
            fastFall = true;
        }
        if (fastFall) {
            nextPiece |= tetris.step();
        }
        if (nextPiece) {
            fastFall = false;
            nextPiece = false;
        }
        prevKP3State = window.getKey(GLFW_KEY_KP_3);
        prevKP1State = window.getKey(GLFW_KEY_KP_1);
        prevKP6State = window.getKey(GLFW_KEY_KP_6);
        prevKP2State = window.getKey(GLFW_KEY_KP_2);
        cursor = window.getCursorPos();

        wait = copyState(tetris, meshes[trunk].obj<Trunk>());
    }
    return 0;
}
