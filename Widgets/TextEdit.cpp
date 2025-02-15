#include "TextEdit.h"

#include "../Text.h"
#include "../Keyboard.h"
#include <cctype>
#include <map>

const SDL_Scancode printableKeys[] = {
    SDL_SCANCODE_SPACE, SDL_SCANCODE_APOSTROPHE, SDL_SCANCODE_COMMA, SDL_SCANCODE_MINUS, SDL_SCANCODE_PERIOD, SDL_SCANCODE_SLASH,
    SDL_SCANCODE_0, SDL_SCANCODE_1, SDL_SCANCODE_2, SDL_SCANCODE_3, SDL_SCANCODE_4, SDL_SCANCODE_5, SDL_SCANCODE_6, SDL_SCANCODE_7,
    SDL_SCANCODE_8, SDL_SCANCODE_9, SDL_SCANCODE_SEMICOLON, SDL_SCANCODE_EQUALS, SDL_SCANCODE_A, SDL_SCANCODE_B, SDL_SCANCODE_C,
    SDL_SCANCODE_D, SDL_SCANCODE_E, SDL_SCANCODE_F, SDL_SCANCODE_G, SDL_SCANCODE_H, SDL_SCANCODE_I, SDL_SCANCODE_J, SDL_SCANCODE_K,
    SDL_SCANCODE_L, SDL_SCANCODE_M, SDL_SCANCODE_N, SDL_SCANCODE_O, SDL_SCANCODE_P, SDL_SCANCODE_Q, SDL_SCANCODE_R, SDL_SCANCODE_S,
    SDL_SCANCODE_T, SDL_SCANCODE_U, SDL_SCANCODE_V, SDL_SCANCODE_W, SDL_SCANCODE_X, SDL_SCANCODE_Y, SDL_SCANCODE_Z,
    SDL_SCANCODE_BACKSLASH, SDL_SCANCODE_GRAVE
};

const std::map<int, char> toUpperMap = {
    { SDLK_1, '!' },
    { SDLK_2, '@' },
    { SDLK_3, '#' },
    { SDLK_4, '$' },
    { SDLK_5, '%' },
    { SDLK_6, '^' },
    { SDLK_7, '&' },
    { SDLK_8, '*' },
    { SDLK_9, '(' },
    { SDLK_0, ')' },
};

char withShift(int key) {
    auto it = toUpperMap.find(key);
    if (it != end(toUpperMap)) {
        return (char)it->second;
    }
    return std::toupper(key);
}

TextEdit::TextEdit(Keyboard* keyboard, Text* text)
    : _keys(keyboard), _line(text, 0.06f), _cursor(0), _shown(false)
{
    _keys->onAdvance([=, this] (auto window, auto state) {
        (void)window;
        if (state != State::NameInput)
            return;

        for (auto key : printableKeys) {
            auto keyState = keyboard->keys().at(key);
            if (keyState == KeyState::Press && _keyStates[key].prev == KeyState::Release) {
                if (_text.size() > 11)
                    return;
                auto isShiftPressed =
                    keyboard->keys().at(SDL_SCANCODE_LSHIFT) == KeyState::Press ||
                    keyboard->keys().at(SDL_SCANCODE_RSHIFT) == KeyState::Press;
                auto code = SDL_GetKeyFromScancode(key);
                _text += isShiftPressed ? withShift(code) : code;
                _cursor++;
            }
            _keyStates[key].prev = keyState;
        }

        auto keyState = keyboard->keys().at(SDL_SCANCODE_BACKSPACE);
        if (keyState == KeyState::Press && _keyStates[SDL_SCANCODE_BACKSPACE].prev == KeyState::Release) {
            if (!_shown || _text.empty() || _cursor == 0)
                return;
            _text = _text.substr(0, _cursor - 1) + _text.substr(_cursor);;
            _cursor = std::max(0, _cursor - 1);
        }
        _keyStates[SDL_SCANCODE_BACKSPACE].prev = keyState;
    });
}

void TextEdit::animate(fseconds)
{ }

void TextEdit::draw() {
    if (_shown) {
        _line.set(_text);
        _line.draw();
    }
}

void TextEdit::measure(glm::vec2 available, glm::vec2 framebuffer) {
    _line.measure(available, framebuffer);
}

void TextEdit::arrange(glm::vec2 pos, glm::vec2 size) {
    _line.arrange(pos, size);
}

glm::vec2 TextEdit::desired() {
    return _line.desired();
}

void TextEdit::setTransform(glm::mat4 transform) {
    _line.setTransform(transform);
}

void TextEdit::show(bool on) {
    _shown = on;
    _text.clear();
}

std::string TextEdit::text() const {
    return _text;
}
