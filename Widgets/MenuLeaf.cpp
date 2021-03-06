#include "MenuLeaf.h"


MenuLeaf::MenuLeaf(Text *textCache, std::vector<std::string> values, std::string line, float relHeight)
    : _values(values), _line(textCache, relHeight), _lineText(line)
{
    _line.set(line);
}

void MenuLeaf::updateValues(std::vector<std::string> values, std::string selected) {
    _values = values;
    setValue(selected);
}

void MenuLeaf::setValue(std::string value) {
    _value = value;
    _line.set(_lineText + ": " + value);
}

const std::vector<std::string> &MenuLeaf::values() const {
    return _values;
}

std::string MenuLeaf::value() {
    return _value;
}

void MenuLeaf::highlight(bool on) {
    _line.highlight(on);
}

void MenuLeaf::animate(fseconds) { }

void MenuLeaf::draw() {
    _line.draw();
}

void MenuLeaf::measure(glm::vec2 available, glm::vec2 framebuffer) {
    _line.measure(available, framebuffer);
}

void MenuLeaf::arrange(glm::vec2 pos, glm::vec2 size) {
    _line.arrange(pos, size);
}

glm::vec2 MenuLeaf::desired() {
    return _line.desired();
}

void MenuLeaf::setTransform(glm::mat4 transform) {
    _line.setTransform(transform);
}

float MenuLeaf::width() {
    return desired().x;
}
