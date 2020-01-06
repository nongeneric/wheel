#include "Keyboard.h"
#include "rstd.h"

void Keyboard::invokeHandler(
    InputCommand command, const std::map<InputCommand, std::vector<Keyboard::Handler>>& handlers)
{
    auto it = handlers.find(command);
    if (it != end(handlers)) {
        for (Handler const& h : it->second) {
            h();
        }
    }
}

void Keyboard::readGamepadState() {
    rstd::fill(_gamepad.buttons, GLFW_RELEASE);
    for (auto n = GLFW_JOYSTICK_1; n != GLFW_JOYSTICK_LAST; ++n) {
        if (glfwGetGamepadState(n, &_gamepad)) {
            _gamepadName = glfwGetGamepadName(n);
            return;
        }
    }
}

std::tuple<int, int> Keyboard::readCommandState(InputCommand command) {
    auto window = _window->handle();

    if (auto it = _keyBindings.find(command); it != _keyBindings.end()) {
        auto key = it->second;
        auto state = glfwGetKey(window, key);
        int prev = state;
        std::swap(prev, _sharedPrevKeyStates[key]);
        if (state == GLFW_PRESS) {
            _inputDeviceName = "Keyboard";
            return {prev, state};
        }
    }

    if (auto it = _gamepadBindings.find(command); it != _gamepadBindings.end()) {
        auto button = it->second;
        int prev = _gamepad.buttons[button];
        std::swap(prev, _sharedPrevKeyStates[button]);
        if (_gamepad.buttons[button] == GLFW_PRESS) {
            _inputDeviceName = _gamepadName;
            return {prev, _gamepad.buttons[button]};
        }
    }

    return {GLFW_RELEASE, GLFW_RELEASE};
}

Keyboard::Keyboard(Window *window) : _window(window) {
    _keyBindings = {
        {InputCommand::MoveLeft, GLFW_KEY_LEFT},
        {InputCommand::MoveRight, GLFW_KEY_RIGHT},
        {InputCommand::RotateLeft, GLFW_KEY_Z},
        {InputCommand::RotateRight, GLFW_KEY_X},
        {InputCommand::RotateRightAlt, GLFW_KEY_UP},
        {InputCommand::MoveUp, GLFW_KEY_UP},
        {InputCommand::MoveDown, GLFW_KEY_DOWN},
        {InputCommand::OpenMenu, GLFW_KEY_ESCAPE},
        {InputCommand::GoBack, GLFW_KEY_ESCAPE},
        {InputCommand::Confirm, GLFW_KEY_ENTER},
    };

    _gamepadBindings = {
        {InputCommand::MoveLeft, GLFW_GAMEPAD_BUTTON_DPAD_LEFT},
        {InputCommand::MoveRight, GLFW_GAMEPAD_BUTTON_DPAD_RIGHT},
        {InputCommand::RotateLeft, GLFW_GAMEPAD_BUTTON_SQUARE},
        {InputCommand::RotateRight, GLFW_GAMEPAD_BUTTON_CROSS},
        {InputCommand::MoveUp, GLFW_GAMEPAD_BUTTON_DPAD_UP},
        {InputCommand::MoveDown, GLFW_GAMEPAD_BUTTON_DPAD_DOWN},
        {InputCommand::OpenMenu, GLFW_GAMEPAD_BUTTON_START},
        {InputCommand::GoBack, GLFW_GAMEPAD_BUTTON_CIRCLE},
        {InputCommand::Confirm, GLFW_GAMEPAD_BUTTON_CROSS},
    };

    // assume the keyboard and gamepad keys are different
    assert(_gamepadBindings.rbegin()->second < _keyBindings.begin()->second);
}

void Keyboard::advance(fseconds dt) {
    readGamepadState();
    if (_onAdvanceHandler) {
        _onAdvanceHandler(_window, _currentState);
    }
    for (auto& [command, state] : _stateSpecificKeyStates[_currentState]) {
        auto [prevState, curState] = readCommandState(command);
        if (curState == GLFW_PRESS && prevState == GLFW_RELEASE) {
            invokeHandler(command, _stateDownHandlers[_currentState]);
            invokeHandler(command, _stateRepeatHandlers[_currentState]);
            _activeRepeats[command] = true;
            state.elapsed = fseconds();
        }
        if (curState == GLFW_PRESS &&
            prevState == GLFW_PRESS &&
            state.elapsed > state.repeat &&
            _activeRepeats[command])
        {
            state.elapsed -= state.repeat;
            invokeHandler(command, _stateRepeatHandlers[_currentState]);
        }
        state.elapsed += dt;
    }
}

void Keyboard::onDown(InputCommand command, State handlerId, std::function<void()> handler) {
    auto it = _stateSpecificKeyStates[handlerId].find(command);
    if (it == end(_stateSpecificKeyStates[handlerId])) {
        _stateSpecificKeyStates[handlerId][command] = { fseconds(), fseconds(1 << 10) };
    }
    _stateDownHandlers[handlerId][command].push_back(handler);
}

void Keyboard::onRepeat(InputCommand command,
                        fseconds every,
                        State handlerId,
                        std::function<void()> handler) {
    _stateSpecificKeyStates[handlerId][command].repeat = every;
    _stateRepeatHandlers[handlerId][command].push_back(handler);
    _activeRepeats[command] = true;
}

void Keyboard::onAdvance(OnAdvanceHandler handler) {
    _onAdvanceHandler = handler;
}

void Keyboard::stopRepeats(InputCommand command) {
    auto [_, state] = readCommandState(command);
    if (state == GLFW_PRESS) {
        _activeRepeats[command] = false;
    }
}

void Keyboard::enableHandler(State id) {
    _currentState = id;
}

const std::string& Keyboard::inputDeviceName() const {
    return _inputDeviceName;
}

std::string strState(State state) {
    switch (state) {
    case State::Game: return "Game";
    case State::HighScores: return "HighScores";
    case State::Menu: return "Menu";
    case State::NameInput: return "NameInput";
    default: assert(false); return "";
    }
}
