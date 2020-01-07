#pragma once

#include "IWidget.h"
#include "SpreadAnimator.h"
#include <memory>

class Text;
class MenuLeaf;
class Menu : public IWidget {
    static constexpr float _lineHeight = 1.2f;
    static constexpr float _charRelHeight = 0.05f;

    std::vector<std::unique_ptr<MenuLeaf>> _leafs;
    std::string _text;
    Text* _textCache;
    glm::vec2 _pos;
    Menu* _parent = nullptr;
    //std::vector<LeafPos> _leafPositions;
    //bool _animating = false;
    //fseconds _elapsed;
    //bool _isAssembling;
    glm::vec2 _size;
    std::unique_ptr<SpreadAnimator> _animator;
    std::vector<ISpreadAnimationLine*> leafsAsAnimationLines();
public:
    Menu(Text* textCache);
    void animate(bool assemble);
    void addLeaf(MenuLeaf* leaf);
    std::vector<MenuLeaf*> leafs();
    Menu* parent();
    glm::vec2 size();
    void animate(fseconds dt) override;
    void draw() override;
    void measure(glm::vec2 available, glm::vec2 framebuffer) override;
    void arrange(glm::vec2 pos, glm::vec2) override;
    glm::vec2 desired() override;
    void setTransform(glm::mat4) override;
};
