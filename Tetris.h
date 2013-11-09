#pragma once

#include <functional>
#include <memory>
#include <assert.h>

namespace PieceType {
    enum t {
        I, J, L, O, S, T, Z, count
    };
}

enum class CellState {
    Hidden, Shown, Dying
};

struct CellInfo {
    CellState state;
    PieceType::t piece;
    inline CellInfo(CellState s = CellState::Hidden, PieceType::t p = PieceType::I)
        : state(s), piece(p) { }
};

struct TetrisStatistics {
    unsigned lines = 0;
    unsigned score = 0;
    unsigned level = 0;
    bool gameOver = false;
};

class Tetris {
    class impl;
    std::unique_ptr<impl> _impl;
public:
    Tetris(int hor, int vert, std::function<PieceType::t()> generator);
    void setInitialLevel(int level);
    CellInfo getState(int x, int y) const;
    CellInfo getNextPieceState(int x, int y) const; // 4x4
    bool step();
    void moveRight();
    void moveLeft();
    void rotate();
    void collect();
    void reset();
    TetrisStatistics getStats() const;
    ~Tetris();
};
