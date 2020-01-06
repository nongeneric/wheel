#pragma once

#include "IWidget.h"
#include "TextLine.h"

#include <string>
#include <map>

class Keyboard;
class Text;
class TextEdit : public IWidget {
    struct KeyState {
        int current;
        int prev;
    };

    Keyboard* _keys;
    TextLine _line;
    std::string _text;
    int _cursor;
    bool _shown;
    std::map<int, KeyState> _keyStates;

public:
    TextEdit(Keyboard* keyboard, Text* text);
    void animate(fseconds dt) override;
    void draw() override;
    void measure(glm::vec2 available, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2 size) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4 transform) override;
    void show(bool on);
    std::string text() const;
};
