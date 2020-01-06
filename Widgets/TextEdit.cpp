#include "TextEdit.h"

#include "../Text.h"
#include "../Keyboard.h"
#include <cctype>
#include <map>

const int printableKeys[] = {
    GLFW_KEY_SPACE, GLFW_KEY_APOSTROPHE, GLFW_KEY_COMMA, GLFW_KEY_MINUS, GLFW_KEY_PERIOD, GLFW_KEY_SLASH,
    GLFW_KEY_0, GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4, GLFW_KEY_5, GLFW_KEY_6, GLFW_KEY_7,
    GLFW_KEY_8, GLFW_KEY_9, GLFW_KEY_SEMICOLON, GLFW_KEY_EQUAL, GLFW_KEY_A, GLFW_KEY_B, GLFW_KEY_C,
    GLFW_KEY_D, GLFW_KEY_E, GLFW_KEY_F, GLFW_KEY_G, GLFW_KEY_H, GLFW_KEY_I, GLFW_KEY_J, GLFW_KEY_K,
    GLFW_KEY_L, GLFW_KEY_M, GLFW_KEY_N, GLFW_KEY_O, GLFW_KEY_P, GLFW_KEY_Q, GLFW_KEY_R, GLFW_KEY_S,
    GLFW_KEY_T, GLFW_KEY_U, GLFW_KEY_V, GLFW_KEY_W, GLFW_KEY_X, GLFW_KEY_Y, GLFW_KEY_Z,
    GLFW_KEY_LEFT_BRACKET, GLFW_KEY_BACKSLASH, GLFW_KEY_RIGHT_BRACKET, GLFW_KEY_GRAVE_ACCENT
};

const std::map<int, char> toUpperMap = {
    { GLFW_KEY_1, '!' },
    { GLFW_KEY_2, '@' },
    { GLFW_KEY_3, '#' },
    { GLFW_KEY_4, '$' },
    { GLFW_KEY_5, '%' },
    { GLFW_KEY_6, '^' },
    { GLFW_KEY_7, '&' },
    { GLFW_KEY_8, '*' },
    { GLFW_KEY_9, '(' },
    { GLFW_KEY_0, ')' },
};

char withShift(int key) {
    auto it = toUpperMap.find(key);
    if (it != end(toUpperMap)) {
        return (char)it->second;
    }
    return (char)key;
}

TextEdit::TextEdit(Keyboard* keyboard, Text* text)
    : _keys(keyboard), _line(text, 0.06f), _cursor(0), _shown(false)
{
    _keys->onAdvance([=] (auto window, auto state) {
        if (state != State::NameInput)
            return;

        for (unsigned key : printableKeys) {
            auto keyState = glfwGetKey(window->handle(), key);
            if (keyState == GLFW_PRESS && _keyStates[key].prev == GLFW_RELEASE) {
                if (_text.size() > 11)
                    return;
                auto isShiftPressed =
                    glfwGetKey(window->handle(), GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS ||
                    glfwGetKey(window->handle(), GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
                _text += isShiftPressed ? withShift(key) : (char)std::tolower(key);
                _cursor++;
            }
            _keyStates[key].prev = keyState;
        }

        auto keyState = glfwGetKey(window->handle(), GLFW_KEY_BACKSPACE);
        if (keyState == GLFW_PRESS && _keyStates[GLFW_KEY_BACKSPACE].prev == GLFW_RELEASE) {
            if (!_shown || _text.empty() || _cursor == 0)
                return;
            _text = _text.substr(0, _cursor - 1) + _text.substr(_cursor);;
            _cursor = std::max(0, _cursor - 1);
        }
        _keyStates[GLFW_KEY_BACKSPACE].prev = keyState;
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
