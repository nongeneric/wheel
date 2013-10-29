#include "Menu.h"

#include "../Text.h"
#include "../rstd.h"
#include "MenuLeaf.h"

std::vector<ISpreadAnimationLine *> Menu::leafsAsAnimationLines() {
    return rstd::fmap<std::unique_ptr<MenuLeaf>, ISpreadAnimationLine*>(_leafs, [](std::unique_ptr<MenuLeaf> const& leaf) {
        return leaf.get();
    });
}

Menu::Menu(Text *textCache) : _textCache(textCache) { }

void Menu::addLeaf(MenuLeaf *leaf)
{
    _leafs.push_back(std::unique_ptr<MenuLeaf>(leaf));
}

std::vector<MenuLeaf *> Menu::leafs() {
    return rstd::fmap<std::unique_ptr<MenuLeaf>, MenuLeaf*>(_leafs, [](const std::unique_ptr<MenuLeaf>& ptr) { return ptr.get(); });
}

Menu *Menu::parent() {
    return _parent;
}

glm::vec2 Menu::size() {
    return _size;
}

void Menu::animate(fseconds dt) {
    _animator->animate(dt);
}

void Menu::draw() {
    for (auto const& leaf : _leafs) {
        leaf->draw();
    }
}

void Menu::measure(glm::vec2 available, glm::vec2 framebuffer) {
    if (!_animator) {
        _animator.reset(new SpreadAnimator(leafsAsAnimationLines()));
    }
    for (auto const& leaf : _leafs) {
        leaf->measure(glm::vec2 { 0, 0 }, framebuffer);
    }

    float y = 0;
    for (auto& leaf : _leafs)
        y += leaf->desired().y * 1.2;
    _size = glm::vec2 { _animator->calcWidthAfterMeasure(available.x), y };
}

void Menu::arrange(glm::vec2 pos, glm::vec2) {
    auto centers = _animator->getCenters();
    float y = _size.y;
    for (unsigned i = 0; i < _leafs.size(); ++i) {
        y -= _leafs[i]->desired().y * 1.2;
        _leafs[i]->arrange(pos + glm::vec2 { centers[i], y }, _leafs[i]->desired());
    }
}

glm::vec2 Menu::desired() {
    return _size;
}

void Menu::setTransform(glm::mat4) { }

void Menu::animate(bool assemble) {
    if (!_animator) {
        _animator.reset(new SpreadAnimator(leafsAsAnimationLines()));
    }
    _animator->beginAnimating(assemble);
}
