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

enum class InputCommand {
    MoveLeft,
    MoveRight,
    MoveUp,
    MoveDown,
    RotateLeft,
    RotateRight,
    RotateRightAlt,
    OpenMenu,
    GoBack,
    Confirm,
    EnterPrintableChar,
};

std::string strState(State);

using OnAdvanceHandler = std::function<void(Window*, State)>;

class Keyboard {
    struct KeyState {
        fseconds elapsed;
        fseconds repeat;
    };
    typedef std::function<void()> Handler;
    State _currentState;
    fseconds _repeatTime = fseconds(0.3f);
    std::map<InputCommand, int> _sharedPrevKeyStates;
    std::map<State, std::map<InputCommand, KeyState>> _stateSpecificKeyStates;
    std::map<State, std::map<InputCommand, std::vector<Handler>>> _stateDownHandlers;
    std::map<State, std::map<InputCommand, std::vector<Handler>>> _stateRepeatHandlers;
    std::map<InputCommand, bool> _activeRepeats;
    std::map<InputCommand, int> _keyBindings;
    std::map<InputCommand, int> _gamepadBindings;
    Window* _window;
    GLFWgamepadstate _gamepad;
    std::string _gamepadName;
    OnAdvanceHandler _onAdvanceHandler;
    std::string _inputDeviceName;

    void invokeHandler(InputCommand command, std::map<InputCommand, std::vector<Handler>> const& handlers);
    void readGamepadState();
    int readCommandState(InputCommand command);

public:
    Keyboard(Window* window);
    void advance(fseconds dt);
    void onDown(InputCommand command, State handlerId, std::function<void()> handler);
    void onRepeat(InputCommand command, fseconds every, State handlerId, std::function<void()> handler);
    void onAdvance(OnAdvanceHandler handler);
    void stopRepeats(InputCommand command);
    void enableHandler(State id);
    void disableHandler(State id);
    const std::string& inputDeviceName() const;
};
