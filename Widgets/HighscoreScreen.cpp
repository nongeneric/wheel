#include "HighscoreScreen.h"

#include "../Text.h"

#include <numeric>

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
        w += tl.desired().x;
    return w;
}

void HighscoreScreen::HighscoreLine::setTransform(glm::mat4 m) {
    for (auto& tl : textLines)
        tl.setTransform(m);
}

void HighscoreScreen::HighscoreLine::highlight(bool on) {
    for (auto& tl : textLines)
        tl.highlight(on);
}

std::vector<ISpreadAnimationLine *> HighscoreScreen::asAnimationLines() {
    std::vector<ISpreadAnimationLine*> res;
    for (auto& line : _lines)
        res.push_back(&line);
    return res;
}

void HighscoreScreen::clearHighlights() {
    for (unsigned i = 0; i < _lines.size() - 1; ++i) {
        _lines[i].highlight(false);
    }
}

HighscoreScreen::HighscoreScreen(Text *text, TetrisConfig* config)
    : _text(text)
{
    _strName = config->string(StringID::HallOfFame_Name);
    _strLines = config->string(StringID::HallOfFame_Lines);
    _strScore = config->string(StringID::HallOfFame_Score);
    _strLevel = config->string(StringID::HallOfFame_Level);
}

void HighscoreScreen::setRecords(std::vector<HighscoreRecord> records) {
    _lines.clear();
    int i = 1;
    for (HighscoreRecord rec : records) {
        _lines.push_back(HighscoreLine(_text, {
            std::to_string(i++),
            rec.name,
            std::to_string(rec.lines),
            std::to_string(rec.score),
            std::to_string(rec.initialLevel)
        }));
    }
    std::ranges::reverse(_lines);
    _lines.push_back(HighscoreLine(_text, { "#", _strName, _strLines, _strScore, _strLevel }));
    _lines.back().highlight(true);
    _animator.reset(new SpreadAnimator(asAnimationLines()));
    clearHighlights();
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
    _xMargin = 0.02 * framebuffer.x;
    _yMargin = 0.02 * framebuffer.y;
    _columnWidths.resize(5);
    std::ranges::fill(_columnWidths, 0);
    float y = 0;
    for (HighscoreLine& line : _lines) {
        for (unsigned i = 0; i < line.columns.size(); ++i) {
            line.textLines[i].measure(glm::vec2{}, framebuffer);
            _columnWidths[i] = std::max(_columnWidths[i], line.textLines[i].desired().x);
        }
        y += line.textLines.front().desired().y + _yMargin;
    }
    float xMargins = (_columnWidths.size() - 1) * _xMargin;
    _desired = glm::vec2 { std::accumulate(begin(_columnWidths), end(_columnWidths), .0f) + xMargins, y };
    _animator->calcWidthAfterMeasure(framebuffer.x);
}

void HighscoreScreen::arrange(glm::vec2 pos, glm::vec2) {
    float y = 0;
    for (HighscoreLine& line : _lines) {
        float x = 0;
        for (unsigned i = 0; i < line.columns.size(); ++i) {
            line.textLines[i].arrange(pos + glm::vec2 { x, y }, line.textLines[i].desired());
            x += _columnWidths[i] + (i != line.columns.size() - 1 ? _xMargin : 0);
        }
        y += line.textLines.front().desired().y + _yMargin;
    }
}

glm::vec2 HighscoreScreen::desired() {
    return _desired;
}

void HighscoreScreen::setTransform(glm::mat4) { }

void HighscoreScreen::highlight(int i) {
    clearHighlights();
    _lines.at(_lines.size() - i - 2).highlight(true);
}
