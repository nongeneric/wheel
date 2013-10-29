#pragma once

#include "IWidget.h"
#include "SpreadAnimator.h"
#include "TextLine.h"

struct HighscoreRecord {
    std::string name;
    int lines;
    int score;
    int initialLevel;
};

class Text;
class HighscoreScreen : public IWidget {
    struct HighscoreLine : public ISpreadAnimationLine {
        HighscoreLine(Text* text, std::vector<std::string> columns);
        std::vector<TextLine> textLines;
        std::vector<std::string> columns;
        float width() override;
        void setTransform(glm::mat4 m);
    };
    std::vector<HighscoreLine> _lines;
    glm::vec2 _desired;
    std::vector<float> _columnWidths;
    float _xOffset;
    std::unique_ptr<SpreadAnimator> _animator;
    std::vector<ISpreadAnimationLine*> asAnimationLines();
public:
    HighscoreScreen(std::vector<HighscoreRecord> records, Text* text);
    void beginAnimating(bool assemble);
    void animate(fseconds dt) override;
    void draw();
    void measure(glm::vec2, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4) override;
};
