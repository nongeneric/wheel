#pragma once

#include "TextLine.h"
#include <vector>

class HudList : public IWidget {
    std::vector<TextLine> _lines;
    Text* _text;
    glm::vec2 _desired;
public:
    HudList(int lines, Text* text, float relHeight);
    void setLine(int i, std::string text);
    void animate(fseconds) override;
    void draw();
    void measure(glm::vec2, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4 transform) override;
};
