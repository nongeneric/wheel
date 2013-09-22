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
    CellState getNextPieceState(int x, int y); // 4x4
    bool step();
    void moveRight();
    void moveLeft();
    void rotate();
    void collect();
    TetrisStatistics getStats();
    ~Tetris();
};
