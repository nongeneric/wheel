#pragma once

#include "CrispBitmap.h"

class Text;
class TextLine : public IWidget {
    Text* _text;
    CrispBitmap _elem;
    std::string _line;
    bool _lineChanged = false;
    glm::vec2 _framebuffer;
    float _relHeight;
    glm::vec2 _pos;
    void updateText();
public:
    TextLine(Text* text, float relHeight);
    void set(std::string line);
    void highlight(bool on);
    void animate(fseconds) override { }
    void draw() override;
    void measure(glm::vec2 size, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4 transform) override;
};
