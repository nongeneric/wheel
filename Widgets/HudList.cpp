#include "HudList.h"


HudList::HudList(int lines, Text *text, float relHeight) : _text(text) {
    for (int i = 0; i < lines; ++i) {
        _lines.emplace_back(text, relHeight);
    }
}

void HudList::setLine(int i, std::string text) {
    _lines[i].set(text);
}

void HudList::animate(fseconds) { }

void HudList::draw() {
    for (TextLine& line : _lines) {
        line.draw();
    }
}

void HudList::measure(glm::vec2, glm::vec2 framebuffer) {
    _desired = glm::vec2 { 0, 0 };
    for (TextLine& line : _lines) {
        line.measure(glm::vec2 { 0, 0 }, framebuffer);
        _desired.x = std::max(_desired.x, line.desired().x);
        _desired.y += line.desired().y;
    }
}

void HudList::arrange(glm::vec2 pos, glm::vec2) {
    float y = 0;
    for (TextLine& line : _lines) {
        line.arrange(pos + glm::vec2 { 0, y }, line.desired());
        y += line.desired().y;
    }
}

glm::vec2 HudList::desired() {
    return _desired;
}

void HudList::setTransform(glm::mat4 transform) {
    for (TextLine& line : _lines)
        line.setTransform(transform);
}
