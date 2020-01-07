#pragma once

#include <map>
#include <stack>
#include <functional>
#include <chrono>

using fseconds = std::chrono::duration<float>;

class Keyboard;
class Menu;
class MenuLeaf;
class MenuController {
    std::map<MenuLeaf*, std::function<void()>> _handlers;
    Menu* _menu;
    MenuLeaf *_leaf;
    std::stack<Menu*> _history;
    bool _customScreen = false;

    void advanceFocus(int delta);
    void clearHighlights();
    void advanceValue(int delta, MenuLeaf* leaf);
public:
    MenuController(Menu* menu, Keyboard* keys);
    void back();
    void backFromCustomScreen();
    void toCustomScreen();
    void onValueChanged(MenuLeaf* leaf, std::function<void()> handler);
    void advance(fseconds dt);
    void setActiveMenu(Menu* menu, bool saveState = true);
    void draw();
    void show();
};
