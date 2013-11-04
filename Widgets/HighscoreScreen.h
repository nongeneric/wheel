#pragma once

#include "IWidget.h"
#include "SpreadAnimator.h"
#include "TextLine.h"
#include "../Config.h"

class Text;
class HighscoreScreen : public IWidget {
    struct HighscoreLine : public ISpreadAnimationLine {
        HighscoreLine(Text* text, std::vector<std::string> columns);
        std::vector<TextLine> textLines;
        std::vector<std::string> columns;
        float width() override;
        void setTransform(glm::mat4 m);
        void highlight(bool on);
    };
    std::vector<HighscoreLine> _lines;
    glm::vec2 _desired;
    std::vector<float> _columnWidths;
    std::unique_ptr<SpreadAnimator> _animator;
    std::vector<ISpreadAnimationLine*> asAnimationLines();
    Text* _text;
    float _xMargin;
    float _yMargin;
    std::string _strName;
    std::string _strLines;
    std::string _strScore;
    std::string _strLevel;
    void clearHighlights();
public:
    HighscoreScreen(Text* text, TetrisConfig* config);
    void setRecords(std::vector<HighscoreRecord> records);
    void beginAnimating(bool assemble);
    void animate(fseconds dt) override;
    void draw();
    void measure(glm::vec2, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4) override;
    void highlight(int i);
};
