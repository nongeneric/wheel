#include "Keyboard.h"

void Keyboard::invokeHandler(int key, const std::map<int, std::vector<Keyboard::Handler> > &handlers) {
    unsigned initialHandlersList = _currentHandlerIds;
    auto it = handlers.find(key);
    if (it != end(handlers)) {
        for (Handler const& h : it->second) {
            if ((unsigned)h.id & initialHandlersList) {
                h.func();
            }
        }
    }
}

Keyboard::Keyboard(Window *window) : _window(window) {  }

void Keyboard::advance(fseconds dt) {
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
            pair.second.elapsed = pair.second.elapsed - pair.second.repeat;
            invokeHandler(pair.first, _repeatHandlers);
        }
        pair.second = { _window->getKey(pair.first),
                        pair.second.elapsed + dt,
                        pair.second.repeat };
    }
}

void Keyboard::onDown(int key, State handlerId, std::function<void ()> handler) {
    auto it = _prevStates.find(key);
    if (it == end(_prevStates)) {
        _prevStates[key] = { GLFW_RELEASE };
    }
    _downHandlers[key].push_back({handlerId, handler});
}

void Keyboard::onRepeat(int key, fseconds every, State handlerId, std::function<void ()> handler) {
    _prevStates[key] = { GLFW_RELEASE, fseconds(), every };
    _repeatHandlers[key].push_back({handlerId, handler});
    _activeRepeats[key] = true;
}

void Keyboard::stopRepeats(int key) {
    if (_window->getKey(key) == GLFW_PRESS) {
        _activeRepeats[key] = false;
    }
}

std::string printActiveHandlers(unsigned ids) {
    std::string res;
    if (ids & (unsigned)State::Game) {
        res += "Game ";
    }
    if (ids & (unsigned)State::HighScores) {
        res += "HighScores ";
    }
    if (ids & (unsigned)State::Menu) {
        res += "Menu ";
    }
    if (ids & (unsigned)State::NameInput) {
        res += "NameInput ";
    }
    return res;
}

void Keyboard::enableHandler(State id) {
    std::cout << "Keyboard's enableHandler: " << printActiveHandlers(_currentHandlerIds) << std::endl;
    _currentHandlerIds |= (unsigned)id;
}

void Keyboard::disableHandler(State id) {
    std::cout << "Keyboard's disableHandler : " << printActiveHandlers(_currentHandlerIds) << std::endl;
    _currentHandlerIds &= ~(unsigned)id;
}

bool Keyboard::isShiftPressed() {
    return _window->getKey(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           _window->getKey(GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
}

Keyboard::ButtonState::ButtonState(int state, fseconds elapsed, fseconds repeat)
    : state(state), elapsed(elapsed), repeat(repeat) { }


std::string strState(State state) {
    switch (state) {
    case State::Game: return "Game";
    case State::HighScores: return "HighScores";
    case State::Menu: return "Menu";
    case State::NameInput: return "NameInput";
    default: assert(false); return "";
    }
}
