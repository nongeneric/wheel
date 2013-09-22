#include "Tetris.h"
#include "Random.h"
#include "rstd.h"
#include <assert.h>

#include <vector>

struct BBox {
    int x, y, size;
};

using Line = std::vector<CellInfo>;
using State = std::vector<Line>;

namespace PieceOrientation {
    enum t {
        Up, Right, Down, Left, count
    };
}

auto _1 = CellInfo { CellState::Shown };
auto _0 = CellInfo { CellState::Hidden };

auto barUp = State {
    Line { _0, _0, _0, _0 },
    Line { _0, _0, _0, _0 },
    Line { _1, _1, _1, _1 },
    Line { _0, _0, _0, _0 },
};

auto barRight = State {
    Line { _0, _0, _1, _0 },
    Line { _0, _0, _1, _0 },
    Line { _0, _0, _1, _0 },
    Line { _0, _0, _1, _0 },
};

auto pieceTup = State {
    Line { _0, _0, _0 },
    Line { _1, _1, _1 },
    Line { _0, _1, _0 },
};

auto pieceTright = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
};

auto pieceTdown = State {
    Line { _0, _1, _0 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceTleft = State {
    Line { _0, _1, _0 },
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceJup = State {
    Line { _1, _1, _1 },
    Line { _0, _0, _1 },
    Line { _0, _0, _0 },
};

auto pieceJright = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _0 },
    Line { _1, _1, _0 },
};

auto pieceJdown = State {
    Line { _1, _0, _0 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceJleft = State {
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceLup = State {
    Line { _1, _1, _1 },
    Line { _1, _0, _0 },
    Line { _0, _0, _0 },
};

auto pieceLright = State {
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceLdown = State {
    Line { _0, _0, _1 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceLleft = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _0 },
    Line { _0, _1, _1 },
};

auto pieceO = State {
    Line { _1, _1 },
    Line { _1, _1 },
};

auto pieceSup = State {
    Line { _0, _0, _0 },
    Line { _0, _1, _1 },
    Line { _1, _1, _0 },
};

auto pieceSright = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _0, _1 },
};

auto pieceZup = State {
    Line { _0, _0, _0 },
    Line { _1, _1, _0 },
    Line { _0, _1, _1 },
};

auto pieceZright = State {
    Line { _0, _0, _1 },
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
};

State pieces[][4] = {
    { barUp, barRight, barUp, barRight },
    { pieceJup, pieceJright, pieceJdown, pieceJleft },
    { pieceLup, pieceLright, pieceLdown, pieceLleft },
    { pieceO, pieceO, pieceO, pieceO },
    { pieceSup, pieceSright, pieceSup, pieceSright },
    { pieceTup, pieceTright, pieceTdown, pieceTleft },
    { pieceZup, pieceZright, pieceZup, pieceZright },
};

int pieceInitialYOffset[] = {
    2, 0, 0, 0, 1, 1, 1
};

class Tetris::impl {
    int _hor, _vert;
    bool _nothingFalling = true;
    State _staticGrid;
    State _dynamicGrid;
    BBox _bbPiece;
    PieceType::t _piece;
    PieceType::t _nextPiece;
    PieceOrientation::t _pieceOrientation;
    Random<unsigned> _random;
    TetrisStatistics _stats;
    State cut(State& source, unsigned posX, unsigned posY, unsigned size) {
        State res(size);
        for (size_t y = 0; y < size; ++y) {
            Line& sourceLine = source.at(y + posY);
            auto sourceLineBegin = begin(sourceLine) + posX;
            auto sourceLineEnd = begin(sourceLine) + posX + size;
            std::copy(sourceLineBegin,
                      sourceLineEnd,
                      std::back_inserter(res.at(y)));
            std::fill(sourceLineBegin,
                      sourceLineEnd,
                      CellInfo { CellState::Hidden });
        }
        return res;
    }

    void paste(State const& piece, State& state, unsigned posX, unsigned posY) {
        for (size_t y = 0; y < piece.size(); ++y) {
            rstd::copy(piece.at(y), begin(state.at(posY + y)) + posX);
        }
    }

    State patchPiece(State const& piece, PieceType::t pieceType) {
        State copy = piece;
        rstd::reverse(copy); // pieces and the grid have different coord systems (y axis)
        for (Line& line : copy) {
            for (CellInfo& cell : line) {
                assert((unsigned)pieceType < PieceType::count);
                cell.piece = pieceType;
            }
        }
        return copy;
    }

    void drawPiece() {
        _pieceOrientation = PieceOrientation::Up;
        _piece = _nextPiece;
        _nextPiece = static_cast<PieceType::t>(_random());
        State piece = patchPiece(pieces[_piece][_pieceOrientation], _piece);
        int yoffset = pieceInitialYOffset[_piece];
        int xoffset = 4 + (_hor - 8) / 2 - piece.size() / 2;
        _bbPiece = { xoffset, _vert - 4 - (int)piece.size() + yoffset, (int)piece.size() };
        paste(piece, _dynamicGrid, _bbPiece.x, _bbPiece.y);
    }

    void nextPiece() {
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
        drawPiece();
        _nothingFalling = false;
    }

    bool collision(State& state) {
        assert(_staticGrid.size() == state.size());
        for (int x = 0; x < _hor; ++x) {
            for (int y = 0; y < _vert; ++y) {
                if (state.at(y).at(x).state == CellState::Shown &&
                        _staticGrid.at(y).at(x).state == CellState::Shown)
                    return true;
            }
        }
        return false;
    }

    void stamp(State const& state) {
        for (int x = 0; x < _hor; ++x) {
            for (int y = 0; y < _vert; ++y) {
                CellInfo cellInfo = state.at(y).at(x);
                if (cellInfo.state == CellState::Shown) {
                    _staticGrid.at(y).at(x) = cellInfo;
                }
            }
        }
    }

    void kill() {
        std::for_each(begin(_staticGrid) + 4, end(_staticGrid), [](Line& line) {
            bool hit = rstd::all_of(line, [](CellInfo cell) {
                    return cell.state == CellState::Shown;
            });
            if (hit) {
                rstd::fill(line, CellInfo { CellState::Dying });
            }
        });
    }

    void updateStats(int lines) {
        _stats.lines += lines;
        int scores[] = { 0, 100, 200, 400, 800 };
        _stats.score += scores[lines];
        _stats.level = _stats.lines / 10;
    }

    void drop() {
        auto res = _dynamicGrid;
        std::copy(begin(res) + 1, end(res), begin(res));
        rstd::fill(res.at(_vert - 1), CellInfo { CellState::Hidden });

        if (collision(res)) {
            stamp(_dynamicGrid);
            _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
            _nothingFalling = true;
        } else {
            _dynamicGrid = res;
            _bbPiece.y -= 1;
        }
    }

    State createState(int width, int height, CellState val) {
        State state;
        state.resize(height);
        for (Line& line : state) {
            line.resize(width);
            rstd::fill(line, CellInfo { val });
        }
        return state;
    }

    void movePieceHor(State& state, int offset) {
        auto box = cut(state, _bbPiece.x, _bbPiece.y, _bbPiece.size);
        paste(box, state, _bbPiece.x + offset, _bbPiece.y);
    }

    void moveHor(int offset) {
        if (_nothingFalling)
            return;
        auto state = _dynamicGrid;
        movePieceHor(state, offset);
        if (!collision(state)) {
            _dynamicGrid = state;
            _bbPiece.x += offset;
        }
    }

    PieceOrientation::t nextOrientation(PieceOrientation::t prev) {
        return static_cast<PieceOrientation::t>((prev + 1) % PieceOrientation::count);
    }
public:
    impl(unsigned hor, unsigned vert)
        : _hor(hor + 8),
          _vert(vert + 8),
          _random(0, PieceType::count - 1)
    {
        _nextPiece = static_cast<PieceType::t>(_random());
        _piece = _nextPiece;
        State inner = createState(hor, vert + 4, CellState::Hidden);
        _staticGrid = createState(_hor, _vert, CellState::Shown);
        paste(inner, _staticGrid, 4, 4);
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
    }

    void collect() {
        auto middle = rstd::stable_partition(_staticGrid, [](Line const& line) {
            return !rstd::all_of(line, [](CellInfo cell) {
                return cell.state == CellState::Dying;
            });
        });
        updateStats(std::distance(middle, end(_staticGrid)));
        Line emptyLine(_hor, { CellState::Shown });
        std::fill(begin(emptyLine) + 4, end(emptyLine) - 4, CellInfo { CellState::Hidden });
        std::fill(middle, end(_staticGrid), emptyLine);
    }

    CellInfo getState(int x, int y) {
        CellInfo& dynInfo = _dynamicGrid.at(y + 4).at(x + 4);
        if (dynInfo.state == CellState::Shown)
            return dynInfo;
        return _staticGrid.at(y + 4).at(x + 4);
    }

    bool step() {
        if (_nothingFalling) {
            nextPiece();
            kill();
            return true;
        }
        drop();
        kill();
        return false;
    }

    void moveRight() {
        moveHor(1);
    }

    void moveLeft() {
        moveHor(-1);
    }

    void rotate() {
        if (_bbPiece.y < 0)
            return;
        auto copy = _dynamicGrid;
        int rightSpike = std::max(0, _bbPiece.x + _bbPiece.size - (_hor - 4));
        int leftSpike = std::max(0, 4 - _bbPiece.x);
        int offset = leftSpike - rightSpike;
        movePieceHor(copy, offset);
        auto newOrient = nextOrientation(_pieceOrientation);
        State nextPiece = patchPiece(pieces[_piece][newOrient], _piece);
        paste(nextPiece, copy, _bbPiece.x + offset, _bbPiece.y);
        if (!collision(copy)) {
            _dynamicGrid = copy;
            _pieceOrientation = newOrient;
            _bbPiece.x += offset;
        }
    }
    TetrisStatistics getStats() {
        return _stats;
    }
    CellInfo getNextPieceState(int x, int y) {
        State state = createState(4, 4, CellState::Hidden);
        assert((unsigned)_nextPiece < PieceType::count);
        State piece = patchPiece(pieces[_nextPiece][PieceOrientation::Up], _nextPiece);
        paste(piece, state, 0, 0);
        assert((unsigned)state.at(y).at(x).piece < PieceType::count);
        return state.at(y).at(x);
    }
};

Tetris::Tetris(int hor, int vert)
    : _impl(new impl(hor, vert))
{ }

CellInfo Tetris::getState(int x, int y) {
    return _impl->getState(x, y);
}

CellInfo Tetris::getNextPieceState(int x, int y) {
    return _impl->getNextPieceState(x, y);
}

bool Tetris::step() {
    return _impl->step();
}

void Tetris::moveRight() {
    _impl->moveRight();
}

void Tetris::moveLeft() {
    _impl->moveLeft();
}

void Tetris::rotate() {
    _impl->rotate();
}

void Tetris::collect() {
    _impl->collect();
}

TetrisStatistics Tetris::getStats() {
    return _impl->getStats();
}

Tetris::~Tetris() { }
