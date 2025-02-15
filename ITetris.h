#pragma once

#include <stdint.h>

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
    CellInfo(CellState s = CellState::Hidden, PieceType::t p = PieceType::I)
        : state(s), piece(p) { }
};

struct TetrisStatistics {
    unsigned lines = 0;
    unsigned score = 0;
    unsigned level = 0;
    bool gameOver = false;
};

struct ITetris {
    virtual void setInitialLevel(int level) = 0;
    virtual CellInfo getState(int x, int y) const = 0;
    virtual CellInfo getNextPieceState(int x, int y) const = 0; // 4x4
    virtual bool step() = 0;
    virtual void moveRight() = 0;
    virtual void moveLeft() = 0;
    virtual void rotate(bool clockwise) = 0;
    virtual int collect() = 0;
    virtual void reset() = 0;
    virtual void resetGameOver() = 0;
    virtual TetrisStatistics getStats() const = 0;
    virtual ~ITetris() = default;
};
