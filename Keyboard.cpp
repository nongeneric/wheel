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
    if (!_gamepad.isConnected()) {
        _gamepad = GameController::GetFirstAvailable();
    }

    SDL_GameControllerUpdate();
    [[maybe_unused]] auto a = _gamepad.button(SDL_CONTROLLER_BUTTON_DPAD_LEFT);
}

std::tuple<KeyState, KeyState> Keyboard::readCommandState(InputCommand command) {
    if (auto it = _keyBindings.find(command); it != _keyBindings.end()) {
        auto key = it->second;
        auto state = _keyboard.at(key);
        auto prev = state;
        std::swap(prev, _sharedPrevKeyStates[key]);
        if (state == KeyState::Press) {
            _inputDeviceName = "Keyboard";
            return {prev, state};
        }
    }

    if (auto it = _gamepadBindings.find(command); it != _gamepadBindings.end()) {
        auto button = it->second;
        auto state = _gamepad.button(button);
        auto prev = state;
        std::swap(prev, _sharedPrevKeyStates[button]);
        if (state == KeyState::Press) {
            _inputDeviceName = _gamepad.name();
            return {prev, state};
        }
    }

    return {KeyState::Release, KeyState::Release};
}

Keyboard::Keyboard(Window *window) : _window(window) {
    int keyboardSize;
    auto keyboard = SDL_GetKeyboardState(&keyboardSize);
    _keyboard = {keyboardSize, keyboard};

    _keyBindings = {
        {InputCommand::MoveLeft, SDL_SCANCODE_LEFT},
        {InputCommand::MoveRight, SDL_SCANCODE_RIGHT},
        {InputCommand::RotateLeft, SDL_SCANCODE_Z},
        {InputCommand::RotateRight, SDL_SCANCODE_X},
        {InputCommand::RotateRightAlt, SDL_SCANCODE_UP},
        {InputCommand::MoveUp, SDL_SCANCODE_UP},
        {InputCommand::MoveDown, SDL_SCANCODE_DOWN},
        {InputCommand::OpenMenu, SDL_SCANCODE_ESCAPE},
        {InputCommand::GoBack, SDL_SCANCODE_ESCAPE},
        {InputCommand::Confirm, SDL_SCANCODE_RETURN},
    };

    _gamepadBindings = {
        {InputCommand::MoveLeft, SDL_CONTROLLER_BUTTON_DPAD_LEFT},
        {InputCommand::MoveRight, SDL_CONTROLLER_BUTTON_DPAD_RIGHT},
        {InputCommand::RotateLeft, SDL_CONTROLLER_BUTTON_X},
        {InputCommand::RotateRight, SDL_CONTROLLER_BUTTON_A},
        {InputCommand::MoveUp, SDL_CONTROLLER_BUTTON_DPAD_UP},
        {InputCommand::MoveDown, SDL_CONTROLLER_BUTTON_DPAD_DOWN},
        {InputCommand::OpenMenu, SDL_CONTROLLER_BUTTON_START},
        {InputCommand::GoBack, SDL_CONTROLLER_BUTTON_B},
        {InputCommand::Confirm, SDL_CONTROLLER_BUTTON_A},
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
        if (curState == KeyState::Press && prevState == KeyState::Release) {
            invokeHandler(command, _stateDownHandlers[_currentState]);
            invokeHandler(command, _stateRepeatHandlers[_currentState]);
            _activeRepeats[command] = true;
            state.elapsed = fseconds();
        }
        if (curState == KeyState::Press &&
            prevState == KeyState::Press &&
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
    if (state == KeyState::Press) {
        _activeRepeats[command] = false;
    }
}

void Keyboard::enableHandler(State id) {
    _currentState = id;
}

const std::string& Keyboard::inputDeviceName() const {
    return _inputDeviceName;
}

const StateArray& Keyboard::keys() const {
    return _keyboard;
}

void Keyboard::rumble(float strength, fseconds duration) {
    _gamepad.rumble(strength, duration);
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

StateArray::StateArray(int size, const Uint8* state)
    : _size(size), _state(reinterpret_cast<const KeyState*>(state)) {}

KeyState StateArray::at(int i) const {
    assert(i < _size);
    return _state[i];
}

GameController::GameController(SDL_GameController* handle, int index)
    : _handle(handle), _index(index) {
    _haptic = SDL_HapticOpen(_index);
    if (_haptic) {
        SDL_HapticRumbleInit(_haptic);
    }
}

GameController::GameController(GameController&& other) {
    *this = std::move(other);
}

GameController& GameController::operator=(GameController&& other) {
    _handle = other._handle;
    _haptic = other._haptic;
    _name = other._name;
    _index = other._index;

    other._handle = nullptr;
    other._haptic = nullptr;

    return *this;
}

GameController GameController::GetFirstAvailable() {
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        if (SDL_IsGameController(i)) {
            auto handle = SDL_GameControllerOpen(i);
            if (handle) {
                return GameController(handle, i);
            }
        }
    }
    return GameController(nullptr, 0);
}

GameController::~GameController() {
    if (_handle) {
        SDL_GameControllerClose(_handle);
    }
    if (_haptic) {
        SDL_HapticClose(_haptic);
    }
}

bool GameController::isOpen() const {
    return _handle;
}

bool GameController::isConnected() const {
    return SDL_GameControllerGetAttached(_handle);
}

std::string GameController::name() const {
    if (_name.empty()) {
        _name = SDL_GameControllerName(_handle);
    }
    return _name;
}

KeyState GameController::button(SDL_GameControllerButton code) const {
    assert(code < SDL_CONTROLLER_BUTTON_MAX);
    if (!isOpen())
        return KeyState::Release;
    return static_cast<KeyState>(SDL_GameControllerGetButton(_handle, code));
}

void GameController::rumble(float strength, fseconds duration) {
    auto ms = chrono::duration_cast<chrono::milliseconds>(duration).count();
    SDL_HapticRumblePlay(_haptic, strength, ms);
}
