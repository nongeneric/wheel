#include "Keyboard.h"

void Keyboard::invokeHandler(int key, const std::map<int, std::vector<Keyboard::Handler> > &handlers) {    
    auto it = handlers.find(key);
    if (it != end(handlers)) {
        for (Handler const& h : it->second) {
            h();
        }
    }
}

Keyboard::Keyboard(Window *window) : _window(window) {  }

void Keyboard::advance(fseconds dt) {    
    for (auto& pair : _stateSpecificKeyStates[_currentState]) {
        int curState = _window->getKey(pair.first);
        int prevState =  _sharedPrevKeyStates[pair.first];
        if (curState == GLFW_PRESS && prevState == GLFW_RELEASE) {
            invokeHandler(pair.first, _stateDownHandlers[_currentState]);            
            invokeHandler(pair.first, _stateRepeatHandlers[_currentState]);
            _activeRepeats[pair.first] = true;
            pair.second.elapsed = fseconds();
        }
        if (curState == GLFW_PRESS &&
                prevState == GLFW_PRESS &&
                pair.second.elapsed > pair.second.repeat &&
                _activeRepeats[pair.first])
        {
            pair.second.elapsed -= pair.second.repeat;
            invokeHandler(pair.first, _stateRepeatHandlers[_currentState]);
        }
        _sharedPrevKeyStates[pair.first] = curState;
        pair.second.elapsed += dt;
    }
}

void Keyboard::onDown(int key, State handlerId, std::function<void ()> handler) {
    auto it = _stateSpecificKeyStates[handlerId].find(key);
    if (it == end(_stateSpecificKeyStates[handlerId])) {
        _stateSpecificKeyStates[handlerId][key] = { fseconds(), fseconds(1 << 10) };
    }
    _stateDownHandlers[handlerId][key].push_back(handler);
}

void Keyboard::onRepeat(int key, fseconds every, State handlerId, std::function<void ()> handler) {    
    _stateSpecificKeyStates[handlerId][key].repeat = every;
    _stateRepeatHandlers[handlerId][key].push_back(handler);
    _activeRepeats[key] = true;
}

void Keyboard::stopRepeats(int key) {
    if (_window->getKey(key) == GLFW_PRESS) {
        _activeRepeats[key] = false;
    }
}

void Keyboard::enableHandler(State id) {
    _currentState = id;
}

bool Keyboard::isShiftPressed() {
    return _window->getKey(GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
           _window->getKey(GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
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
