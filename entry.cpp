#include "Random.h"
#include "Window.h"
#include "Program.h"
#include "Tetris.h"
#include "Text.h"
#include "vformat.h"
#include "Config.h"
#include "Keyboard.h"
#include "BindLock.h"
#include "OpenGLbasics.h"
#include "Texture.h"
#include "ScaleAnimation.h"
#include "Mesh.h"
#include "Trunk.h"
#include "Camera.h"
#include "MathTools.h"
#include "HighscoreManager.h"

#include "Widgets/SpreadAnimator.h"
#include "Widgets/IWidget.h"
#include "Widgets/CrispBitmap.h"
#include "Widgets/TextLine.h"
#include "Widgets/HudList.h"
#include "Widgets/MenuLeaf.h"
#include "Widgets/Menu.h"
#include "Widgets/HighscoreScreen.h"
#include "Widgets/MenuController.h"
#include "Widgets/Painter2D.h"
#include "Widgets/TextEdit.h"

#include <cstring>

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
#include <array>

namespace chrono = boost::chrono;

const int g_TetrisHor = 10;
const int g_TetrisVert = 20;

class Mesh;
void setScale(Mesh& mesh, glm::vec3 scale);

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

bool loadConfig(TetrisConfig& config) {
    try {
        config.load();
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
    program.bindAttribLocation(0, "position");
    program.bindAttribLocation(1, "texCoord");
    program.bindAttribLocation(2, "normal");
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

struct MainMenuStructure {
    MenuLeaf* resume;
    MenuLeaf* restart;
    MenuLeaf* options;
    MenuLeaf* hallOfFame;
    MenuLeaf* exit;
};

MainMenuStructure initMainMenu(Menu& menu, Text& text, TetrisConfig& config) {
    MainMenuStructure res;
    res.resume = new MenuLeaf(&text, {}, config.string(StringID::MainMenu_Resume), 0.05);
    res.restart = new MenuLeaf(&text, {}, config.string(StringID::MainMenu_Restart), 0.05);
    res.options = new MenuLeaf(&text, {}, config.string(StringID::MainMenu_Options), 0.05);
    res.hallOfFame = new MenuLeaf(&text, {}, config.string(StringID::MainMenu_HallOfFame), 0.05);
    res.exit = new MenuLeaf(&text, {}, config.string(StringID::MainMenu_Exit), 0.05);
    menu.addLeaf(res.resume);
    menu.addLeaf(res.restart);
    menu.addLeaf(res.options);
    menu.addLeaf(res.hallOfFame);
    menu.addLeaf(res.exit);
    return res;
}

struct OptionsMenuStructure {
    MenuLeaf* back;
    MenuLeaf* initialSpeed;
    MenuLeaf* monitor;
    MenuLeaf* fullscreen;
    MenuLeaf* resolution;    
};

std::vector<std::string> genNumbers(int count) {
    std::vector<std::string> res;
    for (int i = 0; i < count; ++i)
        res.push_back(vformat("%d", i));
    return res;
}

std::vector<std::string> getMonitorNames() {
    std::vector<std::string> vec;
    for (Monitor& m : getMonitors()) {
        vec.push_back(m.name);
    }
    return vec;
}

std::string printMode(MonitorMode mode) {
    return vformat("%dx%d,%dHz", mode.width, mode.height, mode.refreshRate);
}

std::map<std::string, MonitorMode> getMonitorModes(std::string name) {
    std::map<std::string, MonitorMode> res;
    for (Monitor& monitor : getMonitors()) {
        if (monitor.name != name)
            continue;
        for (MonitorMode& mode : monitor.modes) {
            res[printMode(mode)] = mode;
        }
    }
    return res;
}

std::vector<std::string> selectModeNames(std::map<std::string, MonitorMode> const& modes) {
    std::vector<std::string> res;
    for (auto& pair : modes) {
        res.push_back(pair.first);
    }
    return res;
}

OptionsMenuStructure initOptionsMenu(Menu& menu, TetrisConfig& config, Text& text) {
    OptionsMenuStructure res;
    int speed = std::min(config.initialLevel, 19u);
    res.back = new MenuLeaf(&text, {}, config.string(StringID::OptionsMenu_Back), 0.05);
    std::string strYes = config.string(StringID::Menu_Yes);
    std::string strNo = config.string(StringID::Menu_No);
    (res.initialSpeed = new MenuLeaf(&text, genNumbers(20), config.string(StringID::OptionsMenu_InitialLevel), 0.05f))->setValue(vformat("%d", speed));
    (res.fullscreen = new MenuLeaf(&text, {strYes, strNo}, config.string(StringID::OptionsMenu_Fullscreen), 0.05f))->setValue(config.fullScreen ? strYes : strNo);
    auto monitorNames = getMonitorNames();
    std::string monitor = monitorNames.front();
    (res.monitor = new MenuLeaf(&text, monitorNames, config.string(StringID::OptionsMenu_Monitor), 0.05f))->setValue(monitor);
    auto modes = selectModeNames(getMonitorModes(monitor));
    (res.resolution = new MenuLeaf(&text, modes, config.string(StringID::OptionsMenu_Resolution), 0.05f))->setValue(modes.front());
    menu.addLeaf(res.back);
    menu.addLeaf(res.monitor);
    menu.addLeaf(res.initialSpeed);
    menu.addLeaf(res.fullscreen);
    menu.addLeaf(res.resolution);
    return res;
}

class WindowLayout {
    IWidget* _widget;
    bool _isCentered;
    glm::vec2 _prevFramebuffer;
public:
    WindowLayout(IWidget* widget, bool isCentered)
        : _widget(widget), _isCentered(isCentered) { }
    void updateFramebuffer(glm::vec2 framebuffer) {
//        if (epseq(framebuffer, _prevFramebuffer))
//            return;
        _prevFramebuffer = framebuffer;
        _widget->measure(glm::vec2 {framebuffer.x, .0f}, framebuffer);
        glm::vec2 pos;
        if (_isCentered)
            pos = (framebuffer - _widget->desired()) * 0.5f;
        else
            pos = glm::vec2 { .0f, framebuffer.y - _widget->desired().y };
        _widget->arrange(pos, _widget->desired());
    }
};

class PauseManager {
    Keyboard* _keys;
    bool _paused = false;
public:
    PauseManager(Keyboard* keys) : _keys(keys) { }
    bool paused() const {
        return _paused;
    }
    void flip() {
        _paused = !_paused;
        _keys->disableHandler(_paused ? State::Game : State::Menu);
        _keys->enableHandler(_paused ? State::Menu : State::Game);
    }
};

class HScreenManager {
    HighscoreScreen* _lines;
    HighscoreScreen* _score;
    HighscoreScreen* _current;
    bool _show = false;
public:
    HScreenManager(HighscoreScreen* lines, HighscoreScreen* score)
        : _lines(lines), _score(score), _current(_lines) { }
    void right() {
        if (_current == _lines) {
            _current = _score;
            _current->beginAnimating(true);
        }
    }
    void left() {
        if (_current == _score) {
            _current = _lines;
            _current->beginAnimating(true);
        }
    }
    void draw() {
        if (_show)
            _current->draw();
    }
    void show(bool on, int selectedLinesPos = -1, int selectedScorePos = -1) {
        int selectedPos = -1;
        if (selectedLinesPos != -1) {
            _current = _lines;
            selectedPos = selectedLinesPos;
        } else if (selectedScorePos != -1) {
            _current = _score;
            selectedPos = selectedScorePos;
        }
        if (selectedPos != -1)
            _current->highlight(selectedPos);
        _show = on;
        if (_show) {
            _current->beginAnimating(true);            
        }
    }
    bool show() const {
        return _show;
    }
    void animate(fseconds dt) {
        _current->animate(dt);
    }
};

class GameOverScreen : public IWidget {
    TextEdit _te;
    TextLine _gameOver;
    TextLine _pressEnter;
    TextLine _enterYourName;
    glm::vec2 _desired;
    bool _newHighScore;
    glm::vec2 _framebuffer;
    std::string _strNewHighScore;
    std::string _strGameOver;
    float centerX(float width, float max) {
        return (max - width) / 2;
    }

public:
    GameOverScreen(Keyboard* keys, Text* text, TetrisConfig* config)
        : _te(keys, text), _gameOver(text, 0.07f), _pressEnter(text, 0.03f), _enterYourName(text, 0.045f)
    {
        _strNewHighScore = config->string(StringID::GameOverScreen_NewHighscore);
        _strGameOver = config->string(StringID::GameOverScreen_GameOver);
        _pressEnter.set(config->string(StringID::GameOverScreen_PressEnter));
        _enterYourName.set(config->string(StringID::GameOverScreen_EnterYourName));
    }
    std::string name() {
        return _te.text();
    }
    void animate(fseconds) override { }
    void draw() override {
        _gameOver.draw();
        _pressEnter.draw();
        if (_newHighScore) {
            _te.draw();
            _enterYourName.draw();
        }
    }
    void measure(glm::vec2 available, glm::vec2 framebuffer) override {
        _framebuffer = framebuffer;
        _desired = glm::vec2 { .0f, framebuffer.y };
        for (auto w : std::initializer_list<IWidget*>{ &_te, &_gameOver, &_pressEnter, &_enterYourName }) {
            w->measure(available, framebuffer);
            _desired.x = std::max(_desired.x, w->desired().x);
        }
    }
    void arrange(glm::vec2 pos, glm::vec2) override {
        _gameOver.arrange(pos + glm::vec2 { centerX(_gameOver.desired().x, _desired.x), 0.75f * _framebuffer.y }, _gameOver.desired());
        _enterYourName.arrange(pos + glm::vec2 { centerX(_enterYourName.desired().x, _desired.x), 0.65f * _framebuffer.y }, _enterYourName.desired());
        _pressEnter.arrange(pos + glm::vec2 { centerX(_pressEnter.desired().x, _desired.x), 0.1f * _framebuffer.y }, _pressEnter.desired());
        _te.arrange(pos + glm::vec2 { centerX(_te.desired().x, _desired.x), 0.4f * _framebuffer.y}, _te.desired());

    }
    glm::vec2 desired() override {
        return _desired;
    }
    void setTransform(glm::mat4) override { }
    void show(bool on, bool newHighScore) {        
        _te.show(on);
        _newHighScore = newHighScore;
        _gameOver.set(newHighScore ? _strNewHighScore : _strGameOver);
    }
};

class StateManager {
    State _current;
    PauseManager* _pm;
    MenuController* _mc;
    HScreenManager* _hsm;
    GameOverScreen* _gos;
    Keyboard* _keys;

    int _newLinesHighscorePos;
    int _newScoreHighscorePos;
    std::function<void(std::string)> _nameUpdater;
    void logStateChange(State oldState, State newState) {
        std::cout << "State: " << strState(oldState) << " -> " << strState(newState) << std::endl;
    }
public:
    StateManager(PauseManager* pm, MenuController* mc, HScreenManager* hsm, GameOverScreen* te, Keyboard* keys)
        : _current(State::Game), _pm(pm), _mc(mc), _hsm(hsm), _gos(te), _keys(keys)
    {
        keys->enableHandler(State::Game);
        // State::Game
        _keys->onDown(GLFW_KEY_ESCAPE, State::Game, [&]() {
            goTo(State::Menu);
        });
        // State::Menu
        _mc->onValueChanged(nullptr, [&]() {
            goTo(State::Game);
        });
        // State::NameInput
        _keys->onDown(GLFW_KEY_ENTER, State::NameInput, [&]() {
            if (_newLinesHighscorePos == -1 && _newScoreHighscorePos == -1)
                goTo(State::Menu);
            else
                goToHighscoresState(_newLinesHighscorePos, _newScoreHighscorePos);
        });
        // State::HighScores
        _keys->onDown(GLFW_KEY_ESCAPE, State::HighScores, [&]() {
            goTo(State::Menu);
        });
    }

    void updateKeysHandlers(State newState) {
        _keys->disableHandler(_current);
        _keys->enableHandler(newState);
    }

    void goToHighscoresState(int selectedLinesPos = -1, int selectedScorePos = -1) {
        updateKeysHandlers(State::HighScores);
        if (_current == State::Menu) {
            _hsm->show(true);
            _mc->toCustomScreen();
        } else if (_current == State::NameInput) {
            _nameUpdater(_gos->name());
            _gos->show(false, true);
            _hsm->show(true, selectedLinesPos, selectedScorePos);
        }
        logStateChange(_current, State::HighScores);
        _current = State::HighScores;
    }

    void goToNameInputState(int newLinesHighscorePos, int newScoreHighscorePos, std::function<void(std::string)> nameUpdater) {
        updateKeysHandlers(State::NameInput);
        _newLinesHighscorePos = newLinesHighscorePos;
        _newScoreHighscorePos = newScoreHighscorePos;
        _nameUpdater = nameUpdater;
        if (_current == State::Game) {
           _pm->flip();
           _mc->toCustomScreen();
           _gos->show(true, newLinesHighscorePos != -1 || newScoreHighscorePos != -1);
        } else {
            assert(false);
        }
        logStateChange(_current, State::NameInput);
        _current = State::NameInput;
    }

    void goTo(State state) {
        if (_current == state)
            return;
        updateKeysHandlers(state);
        if (_current == State::Game && state == State::Menu) {
            _pm->flip();
            _mc->show();
        } else if (_current == State::Menu && state == State::Game) {
            _pm->flip();        
        } else if (_current == State::HighScores && state == State::Menu) {
            _hsm->show(false);
            _mc->backFromCustomScreen();
        } else if (_current == State::NameInput && state == State::Menu) {
            _gos->show(false, false);
            _mc->backFromCustomScreen();
        } else {
            assert(false);
        }
        logStateChange(_current, state);
        _current = state;
    }
    void advance(fseconds dt) {
        if (_current == State::Game) {
            // ignore
        } else if (_current == State::HighScores) {
            _hsm->animate(dt);
        } else if (_current == State::Menu) {
            _mc->advance(dt);
        } else if (_current == State::NameInput) {
            _gos->animate(dt);
        }
    }
    void draw() {
        if (_current == State::Game) {
            // ignore
        } else if (_current == State::HighScores) {
            _hsm->draw();
        } else if (_current == State::Menu) {
            _mc->draw();
        } else if (_current == State::NameInput) {
            _gos->draw();
        }
    }
};

int desktop_entry() {
    TetrisConfig config;
    if (!loadConfig(config)) {
        return 1;
    }

    Window window("wheel", config.fullScreen, config.screenWidth, config.screenHeight, config.monitor);
    Keyboard keys(&window);
    PauseManager pm(&keys);
    MainProgramInfo program = createMainProgram();
    std::vector<MeshWrapper> meshes = genMeshes();
    Tetris tetris(g_TetrisHor, g_TetrisVert, Generator(), config.initialLevel);
    Camera camera;
    CameraController camController(&window, &camera, &keys);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0,0,0,0);
    auto past = chrono::high_resolution_clock::now();
    fseconds elapsed;
    fseconds wait;

    Text text;    

    HudList hudList(4, &text, 0.04f);
    WindowLayout hudLayout(&hudList, false);
    fseconds delay = fseconds(1.0f);

    Painter2D p2d;
    p2d.rect(glm::vec2 { 0, 0 }, glm::vec2 { 1.0f, 1.0f }, glm::vec4 {0, 0, 0, 0.7});
    //p2d.rect(glm::vec2 { 0, 0.45f }, glm::vec2 { 0.3f, 0.1f }, glm::vec4 {0.5, 0.5, 0.5, 0.5});

    Menu mainMenu(&text), optionsMenu(&text);
    WindowLayout mainMenuLayout(&mainMenu, true);
    WindowLayout optionsMenuLayout(&optionsMenu, true);
    MainMenuStructure mainMenuStructure = initMainMenu(mainMenu, text, config);
    OptionsMenuStructure optionsMenuStructure = initOptionsMenu(optionsMenu, config, text);

    HighscoreScreen hscreenLines(&text, &config);
    hscreenLines.setRecords(config.highscoreLines);
    HighscoreScreen hscreenScore(&text, &config);
    hscreenScore.setRecords(config.highscoreScore);

    WindowLayout hscreenLinesLayout(&hscreenLines, true);
    WindowLayout hscreenScoreLayout(&hscreenScore, true);
    HScreenManager hscreen(&hscreenLines, &hscreenScore);

    GameOverScreen gameOverScreen(&keys, &text, &config);
    WindowLayout gameOverScreenLayout(&gameOverScreen, true);
    MenuController menu(&mainMenu, &keys);

    StateManager stateManager(&pm, &menu, &hscreen, &gameOverScreen, &keys);

    FpsCounter fps;
    bool canManuallyMove;
    bool normalStep;
    bool nextPiece = false;
    bool exit = false;    
    keys.onRepeat(GLFW_KEY_LEFT, fseconds(0.09f), State::Game, [&]() {
        if (canManuallyMove) {
            tetris.moveLeft();
        }
    });
    keys.onRepeat(GLFW_KEY_RIGHT, fseconds(0.09f), State::Game, [&]() {
        if (canManuallyMove) {
            tetris.moveRight();
        }
    });
    keys.onDown(GLFW_KEY_UP, State::Game, [&]() {
        if (canManuallyMove) {
            tetris.rotate();
        }
    });
    keys.onRepeat(GLFW_KEY_DOWN, fseconds(0.03f), State::Game, [&]() {
        if (!normalStep && canManuallyMove) {
            nextPiece |= tetris.step();
        }
    });
    keys.onDown(GLFW_KEY_LEFT, State::HighScores, [&]() { // TODO: move to highscore
        if (hscreen.show())
            hscreen.left();
    });
    keys.onDown(GLFW_KEY_RIGHT, State::HighScores, [&]() { // TODO: move to highscore
        if (hscreen.show())
            hscreen.right();
    });
    menu.onValueChanged(mainMenuStructure.resume, [&]() {
        stateManager.goTo(State::Game);
    });
    menu.onValueChanged(mainMenuStructure.restart, [&]() {
        wait = fseconds();
        tetris.reset();
        stateManager.goTo(State::Game);
    });
    menu.onValueChanged(mainMenuStructure.options, [&]() {
        menu.setActiveMenu(&optionsMenu);
    });
    menu.onValueChanged(mainMenuStructure.hallOfFame, [&]() {
        stateManager.goToHighscoresState();
    });
    menu.onValueChanged(mainMenuStructure.exit, [&]() {        
        exit = true;
    });
    auto empty = [](){};
    menu.onValueChanged(optionsMenuStructure.back, [&]() { menu.back(); });
    menu.onValueChanged(optionsMenuStructure.fullscreen, empty);
    menu.onValueChanged(optionsMenuStructure.initialSpeed, empty);
    menu.onValueChanged(optionsMenuStructure.resolution, empty);

    while (!window.shouldClose()) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hudList.setLine(0, vformat(config.string(StringID::HUD_Lines), tetris.getStats().lines));
        hudList.setLine(1, vformat(config.string(StringID::HUD_Score), tetris.getStats().score));
        hudList.setLine(2, vformat(config.string(StringID::HUD_Level), tetris.getStats().level));
        if (config.showFps) {
            hudList.setLine(3, vformat(config.string(StringID::HUD_FPS), fps.fps()));
        }

        glm::vec2 framebuffer = window.getFramebufferSize();
        glm::mat4 proj = getProjection(framebuffer, config.orthographic);
        glm::mat4 vpMatrix = proj * camera.view();

        hudLayout.updateFramebuffer(framebuffer);
        mainMenuLayout.updateFramebuffer(framebuffer);
        optionsMenuLayout.updateFramebuffer(framebuffer);
        hscreenLinesLayout.updateFramebuffer(framebuffer);
        hscreenScoreLayout.updateFramebuffer(framebuffer);
        gameOverScreenLayout.updateFramebuffer(framebuffer);

        auto now = chrono::high_resolution_clock::now();
        fseconds dt = chrono::duration_cast<fseconds>(now - past);
        fps.advance(dt);
        fseconds realDt = dt;
        if (pm.paused())
            dt = fseconds();
        wait -= dt;
        elapsed += dt;
        past = now;

        bool waiting = wait > fseconds();

        canManuallyMove = !tetris.getStats().gameOver && !waiting && !pm.paused();
        normalStep = false;
        fseconds levelPenalty(speedCurve(tetris.getStats().level));
        if (elapsed > delay - levelPenalty && canManuallyMove) {
            normalStep = true;
            tetris.collect();
            nextPiece |= tetris.step();
            elapsed -= delay - levelPenalty;
        }

        keys.advance(dt);
        camController.advance();
        if (pm.paused()) {
            menu.advance(realDt);
            hscreen.animate(realDt);
        }

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

        auto stats = tetris.getStats();
        if (stats.gameOver) {
            HighscoreRecord newRecord { "", stats.lines, stats.score, config.initialLevel };
            int newLinesHighscore = updateHighscores(newRecord, config.highscoreLines, true);
            int newScoreHighscore = updateHighscores(newRecord, config.highscoreScore, false);
            stateManager.goToNameInputState(newLinesHighscore, newScoreHighscore, [&](std::string newName) {
                if (newLinesHighscore != -1) {
                    config.highscoreLines.at(newLinesHighscore).name = newName;
                }
                if (newScoreHighscore != -1) {
                    config.highscoreScore.at(newScoreHighscore).name = newName;
                }
                hscreenLines.setRecords(config.highscoreLines);
                hscreenScore.setRecords(config.highscoreScore);
                hscreenLinesLayout.updateFramebuffer(framebuffer);
                hscreenScoreLayout.updateFramebuffer(framebuffer);
                config.save();
            });
            tetris.reset();
        }

        glDisable(GL_DEPTH_TEST);

        hudList.draw();
        if (pm.paused()) {
            p2d.draw();
            stateManager.draw();
        }

        glEnable(GL_DEPTH_TEST);

        window.swap();

        if (exit)
            break;
    }
    config.save();
    return 0;
}
