#pragma once

#include "Window.h"
#include <map>
#include <tuple>
#include <chrono>
#include <functional>

using fseconds = std::chrono::duration<float>;

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

enum class KeyState : Uint8 {
    Release,
    Press,
};

class StateArray {
    int _size;
    const KeyState* _state;
public:
    StateArray() = default;
    StateArray(int size, const Uint8* state);
    KeyState at(int i) const;
};

class GameController {
    SDL_GameController* _handle = nullptr;
    SDL_Haptic* _haptic = nullptr;
    int _index = 0;
    mutable std::string _name;
    GameController(SDL_GameController* handle, int index);

public:
    GameController() = default;
    GameController(const GameController&) = delete;
    GameController& operator=(const GameController&) = delete;
    GameController(GameController&&);
    GameController& operator=(GameController&&);
    ~GameController();
    bool isOpen() const;
    bool isConnected() const;
    std::string name() const;
    KeyState button(SDL_GameControllerButton code) const;
    void rumble(float strength, fseconds duration);
    static GameController GetFirstAvailable();
};

class Keyboard {
    struct KeyInfo {
        fseconds elapsed;
        fseconds repeat;
    };
    typedef std::function<void()> Handler;
    State _currentState;
    fseconds _repeatTime = fseconds(0.3f);
    std::map<int, KeyState> _sharedPrevKeyStates;
    std::map<State, std::map<InputCommand, KeyInfo>> _stateSpecificKeyStates;
    std::map<State, std::map<InputCommand, std::vector<Handler>>> _stateDownHandlers;
    std::map<State, std::map<InputCommand, std::vector<Handler>>> _stateRepeatHandlers;
    std::map<InputCommand, bool> _activeRepeats;
    std::map<InputCommand, int> _keyBindings;
    std::map<InputCommand, SDL_GameControllerButton> _gamepadBindings;
    Window* _window;
    GameController _gamepad;
    StateArray _keyboard;
    OnAdvanceHandler _onAdvanceHandler;
    std::string _inputDeviceName;

    void invokeHandler(InputCommand command, std::map<InputCommand, std::vector<Handler>> const& handlers);
    void readGamepadState();
    std::tuple<KeyState, KeyState> readCommandState(InputCommand command);

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
    const StateArray& keys() const;
    void rumble(float strength, fseconds duration);
};
