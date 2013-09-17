#pragma once

#include <memory>

enum class CellState {
    Shown, Hidden, Dying
};

class Tetris {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Tetris(int hor, int vert);
    CellState getState(int x, int y);
    void step();
    void moveRight();
    void moveLeft();
    void rotate();
    ~Tetris();
};
