#include "HighscoreScreen.h"

#include "../Text.h"
#include "../vformat.h"
#include "../rstd.h"

HighscoreScreen::HighscoreLine::HighscoreLine(Text *text, std::vector<std::string> columns) {
    this->columns = columns;
    for (std::string col : columns) {
        textLines.emplace_back(text, 0.05);
        textLines.back().set(col);
    }
}

float HighscoreScreen::HighscoreLine::width() {
    float w = 0;
    for (auto& tl : textLines)
        w = std::max(w, tl.desired().x);
    return w;
}

void HighscoreScreen::HighscoreLine::setTransform(glm::mat4 m) {
    for (auto& tl : textLines)
        tl.setTransform(m);
}


std::vector<ISpreadAnimationLine *> HighscoreScreen::asAnimationLines() {
    std::vector<ISpreadAnimationLine*> res;
    for (auto& line : _lines)
        res.push_back(&line);
    return res;
}

HighscoreScreen::HighscoreScreen(std::vector<HighscoreRecord> records, Text *text) {
    int i = 1;
    for (HighscoreRecord rec : records) {
        _lines.push_back(HighscoreLine(text, {
            vformat("%d", i++),
            rec.name,
            vformat("%d", rec.lines),
            vformat("%d", rec.score),
            vformat("%d", rec.initialLevel)
        }));
    }
    rstd::reverse(_lines);
    _lines.push_back(HighscoreLine(text, { "#", "Name", "Lines", "Score", "Lvl" }));
    _animator.reset(new SpreadAnimator(asAnimationLines()));
}

void HighscoreScreen::beginAnimating(bool assemble) {
    _animator->beginAnimating(assemble);
}

void HighscoreScreen::animate(fseconds dt) {
    _animator->animate(dt);
}

void HighscoreScreen::draw() {
    for (HighscoreLine& line : _lines) {
        for (TextLine& tline : line.textLines) {
            tline.draw();
        }
    }
}

void HighscoreScreen::measure(glm::vec2, glm::vec2 framebuffer) {
    _columnWidths.resize(5);
    rstd::fill(_columnWidths, 0);
    float y = 0;
    for (HighscoreLine& line : _lines) {
        for (unsigned i = 0; i < line.columns.size(); ++i) {
            line.textLines[i].measure(glm::vec2{}, framebuffer);
            _columnWidths[i] = std::max(_columnWidths[i], line.textLines[i].desired().x);
        }
        y += line.textLines.front().desired().y;
    }
    _desired = glm::vec2 { rstd::accumulate(_columnWidths, .0f), y };
    _xOffset = (framebuffer.x - rstd::accumulate(_columnWidths, .0f)) / 2;
    _animator->calcWidthAfterMeasure(framebuffer.x);
}

void HighscoreScreen::arrange(glm::vec2 pos, glm::vec2) {
    float y = 0;
    for (HighscoreLine& line : _lines) {
        float x = 0;
        for (unsigned i = 0; i < line.columns.size(); ++i) {
            line.textLines[i].arrange(pos + glm::vec2 { x + _xOffset, y }, line.textLines[i].desired());
            x += _columnWidths[i];
        }
        y += line.textLines.front().desired().y;
    }
}

glm::vec2 HighscoreScreen::desired() {
    return _desired;
}

void HighscoreScreen::setTransform(glm::mat4) { }
