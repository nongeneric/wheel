#pragma once

#include <memory>

enum class CellState {
    Shown, Hidden, Dying
};

struct TetrisStatistics {
    unsigned lines = 0;
    unsigned score = 0;
};

class Tetris {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Tetris(int hor, int vert);
    CellState getState(int x, int y);
    bool step();
    void moveRight();
    void moveLeft();
    void rotate();
    TetrisStatistics getStats();
    ~Tetris();
};
