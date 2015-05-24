#include "MenuController.h"

#include "../Keyboard.h"
#include "Menu.h"
#include "MenuLeaf.h"

#include <algorithm>

void MenuController::advanceFocus(int delta) {
    _leaf->highlight(false);
    auto leafs = _menu->leafs();
    auto it = std::find(begin(leafs), end(leafs), _leaf);
    assert(it != end(leafs));
    unsigned index = std::distance(begin(leafs), it);
    index = (index + leafs.size() + delta) % leafs.size();
    _leaf = leafs[index];
    _leaf->highlight(true);
}

void MenuController::clearHighlights() {
    for (auto& leaf : _menu->leafs()) {
        leaf->highlight(false);
    }
    _leaf->highlight(true);
}

void MenuController::advanceValue(int delta, MenuLeaf *leaf) {
    auto const& values = leaf->values();
    auto it = std::find(begin(values), end(values), _leaf->value());
    assert(it != end(values));
    unsigned index = std::distance(begin(values), it);
    index = (index + values.size() + delta) % values.size();
    leaf->setValue(values[index]);
    assert(_handlers.find(_leaf) != end(_handlers));
    _handlers[_leaf]();
}

MenuController::MenuController(Menu *menu, Keyboard *keys)
    : _menu(menu)
{
    setActiveMenu(menu, false);
    clearHighlights();
    keys->onDown(GLFW_KEY_DOWN, State::Menu, [&]() {
        if (_customScreen)
            return;
        advanceFocus(1);
    });
    keys->onDown(GLFW_KEY_UP, State::Menu, [&]() {
        if (_customScreen)
            return;
        advanceFocus(-1);
    });
    keys->onDown(GLFW_KEY_ENTER, State::Menu, [&]() {
        if (_customScreen)
            return;
        if (_leaf->values().empty()) {
            assert(_handlers.find(_leaf) != end(_handlers));
            _handlers[_leaf]();
        }
    });
    keys->onDown(GLFW_KEY_ESCAPE, State::Menu, [&]() { back(); });
    keys->onRepeat(GLFW_KEY_LEFT, fseconds(0.3f), State::Menu, [&]() {
        if (_customScreen)
            return;
        if (_leaf->values().empty())
            return;
        advanceValue(-1, _leaf);
    });
    keys->onRepeat(GLFW_KEY_RIGHT, fseconds(0.3f), State::Menu, [&]() {
        if (_customScreen)
            return;
        if (_leaf->values().empty())
            return;
        advanceValue(1, _leaf);
    });
}

void MenuController::back() {
    if (_customScreen)
        return;
    if (_history.size() > 0) {
        setActiveMenu(_history.top(), false);
        _history.pop();
    } else {
        _handlers[nullptr]();
    }
}

void MenuController::backFromCustomScreen() {
    _customScreen = false;
    back();
}

void MenuController::toCustomScreen() {
    _history.push(_menu);
    _customScreen = true;
}

void MenuController::onValueChanged(MenuLeaf *leaf, std::function<void ()> handler) {
    _handlers[leaf] = handler;
}

void MenuController::advance(fseconds dt) {
    _menu->animate(dt);
}

void MenuController::setActiveMenu(Menu *menu, bool saveState) {
    if (saveState)
        _history.push(_menu);
    _menu = menu;
    _leaf = menu->leafs().front();
    clearHighlights();
    _menu->animate(true);
}

void MenuController::draw() {
    if (!_customScreen) {
        _menu->draw();
    }
}

void MenuController::show() {
    _leaf = _menu->leafs().front();
    _menu->animate(true);
    clearHighlights();
}
