#pragma once

#include "Window.h"
#include <map>
#include <boost/chrono.hpp>

using fseconds = boost::chrono::duration<float>;

enum class State {
    Game = 1,
    Menu = 2,
    NameInput = 4,
    HighScores = 8
};

class Keyboard {
    struct ButtonState {
        int state;
        fseconds elapsed;
        fseconds repeat;
        ButtonState(int state = GLFW_RELEASE, fseconds elapsed = fseconds{}, fseconds repeat = fseconds{});
    };
    struct Handler {
        State id;
        std::function<void()> func;
    };
    fseconds _repeatTime = fseconds(0.3f);
    std::map<int, ButtonState> _prevStates;
    std::map<int, std::vector<Handler>> _downHandlers;
    std::map<int, std::vector<Handler>> _repeatHandlers;    
    std::map<int, bool> _activeRepeats;
    Window* _window;
    unsigned _currentHandlerIds = 0;
    void invokeHandler(int key, std::map<int, std::vector<Handler>> const& handlers);
public:
    Keyboard(Window* window);
    void advance(fseconds dt);
    void onDown(int key, State handlerId, std::function<void()> handler);
    void onRepeat(int key, fseconds every, State handlerId, std::function<void()> handler);
    void stopRepeats(int key);
    void enableHandler(State id);
    void disableHandler(State id);
    bool isShiftPressed();
};
