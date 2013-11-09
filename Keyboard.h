#pragma once

#include "Window.h"
#include <map>
#include <boost/chrono.hpp>

using fseconds = boost::chrono::duration<float>;

enum class State {
    Game,
    Menu,
    NameInput,
    HighScores
};

std::string strState(State);

class Keyboard {
    struct KeyState {
        fseconds elapsed;
        fseconds repeat;        
    };
    typedef std::function<void()> Handler;
    State _currentState;
    fseconds _repeatTime = fseconds(0.3f);
    std::map<int, int> _sharedPrevKeyStates;
    std::map<State, std::map<int, KeyState>> _stateSpecificKeyStates;
    std::map<State, std::map<int, std::vector<Handler>>> _stateDownHandlers;
    std::map<State, std::map<int, std::vector<Handler>>> _stateRepeatHandlers;
    std::map<int, bool> _activeRepeats;    
    Window* _window;
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
