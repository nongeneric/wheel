#pragma once

#include "IWidget.h"
#include "TextLine.h"
#include "SpreadAnimator.h"

class MenuLeaf : public IWidget, public ISpreadAnimationLine {
    std::vector<std::string> _values;
    std::string _value;
    TextLine _line;
    std::string _lineText;
public:
    MenuLeaf(Text* textCache, std::vector<std::string> values, std::string line, float relHeight);
    void setValue(std::string value);
    std::vector<std::string> const& values() const;
    std::string value();
    void highlight(bool on);
    void animate(fseconds) override;
    void draw() override;
    void measure(glm::vec2 available, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2 size) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4 transform) override;
    float width() override;
};
