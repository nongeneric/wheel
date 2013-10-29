#include "TextLine.h"

#include "../MathTools.h"

TextLine::TextLine(Text *text, float relHeight)
    : _text(text), _relHeight(relHeight) { }

void TextLine::set(std::string line) {
    if (line == _line)
        return;
    _lineChanged = true;
    _line = line;
}

void TextLine::setColor(glm::vec3 color) {
    _elem.setColor(color);
}

void TextLine::draw() {
    _elem.draw();
}

void TextLine::measure(glm::vec2 size, glm::vec2 framebuffer) {
    if (!epseq(_framebuffer, framebuffer) || _lineChanged) {
        _lineChanged = false;
        _framebuffer = framebuffer;
        _elem.setBitmap(_text->renderText(_line, framebuffer.y * _relHeight));
    }
    _elem.measure(size, framebuffer);
}

void TextLine::arrange(glm::vec2 pos, glm::vec2) {
    _elem.arrange(pos, desired());
}

glm::vec2 TextLine::desired() {
    return _elem.desired();
}

void TextLine::setTransform(glm::mat4 transform) {
    _elem.setTransform(transform);
}
