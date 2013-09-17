#include "Tetris.h"
#include "Random.h"
#include "rstd.h"
#include <assert.h>

#include <vector>

struct BBox {
    int x, y, size;
};

namespace PieceType {
    enum t {
        I, T, count
    };
}

using Line = std::vector<CellState>;
using State = std::vector<Line>;

namespace PieceOrientation {
    enum t {
        Up, Right, Down, Left
    };
}

std::vector<PieceOrientation::t> OrientationCircle {
    PieceOrientation::Up,
    PieceOrientation::Right,
    PieceOrientation::Down,
    PieceOrientation::Left
};

auto _1 = CellState::Shown;
auto _0 = CellState::Hidden;

auto barUp = State {
    Line { _1, _1, _1, _1 },
    Line { _0, _0, _0, _0 },
    Line { _0, _0, _0, _0 },
    Line { _0, _0, _0, _0 },
};

auto barRight = State {
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
    Line { _0, _1, _0, _0 },
};

auto pieceTup = State {
    Line { _0, _1, _0 },
    Line { _1, _1, _1 },
    Line { _0, _0, _0 },
};

auto pieceTright = State {
    Line { _0, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _1, _0 },
};

auto pieceTdown = State {
    Line { _0, _0, _0 },
    Line { _1, _1, _1 },
    Line { _0, _1, _0 },
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
    Line { _0, _0, _1 },
    Line { _0, _0, _1 },
    Line { _0, _1, _1 },
};

auto pieceJdown = State {
    Line { _0, _0, _0 },
    Line { _1, _0, _0 },
    Line { _1, _1, _1 },
};

auto pieceJleft = State {
    Line { _1, _1, _0 },
    Line { _1, _0, _0 },
    Line { _1, _0, _0 },
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
    Line { _1, _0, _0 },
    Line { _1, _0, _0 },
    Line { _1, _1, _0 },
};

auto pieceO = State {
    Line { _1, _1 },
    Line { _1, _1 },
};

auto pieceSup = State {
    Line { _0, _1, _1 },
    Line { _1, _1, _0 },
    Line { _0, _0, _0 },
};

auto pieceSright = State {
    Line { _1, _0, _0 },
    Line { _1, _1, _0 },
    Line { _0, _1, _0 },
};

auto pieceZup = State {
    Line { _1, _1, _0 },
    Line { _0, _1, _1 },
    Line { _0, _0, _0 },
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

class Tetris::impl {
    int _hor, _vert;
    bool _nothingFalling = true;
    State _staticGrid;
    State _dynamicGrid;
    BBox _bbPiece;
    PieceType::t _piece;
    PieceOrientation::t _pieceOrientation;
    Random<unsigned> _random;
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
                      CellState::Hidden);
        }
        return res;
    }

    void paste(State const& piece, State& state, unsigned posX, unsigned posY) {
        for (size_t y = 0; y < piece.size(); ++y) {
            rstd::copy(piece.at(y), begin(state.at(posY + y)) + posX);
        }
    }

    void drawPiece() {
        _piece = static_cast<PieceType::t>(_random());
        State piece = pieces[_piece][PieceOrientation::Up];
        _bbPiece = { 7, _vert - 4 - (int)piece.size(), 4 };
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
                if (state.at(y).at(x) == CellState::Shown &&
                        _staticGrid.at(y).at(x) == CellState::Shown)
                    return true;
            }
        }
        return false;
    }

    void stamp(State const& state) {
        for (int x = 0; x < _hor; ++x) {
            for (int y = 0; y < _vert; ++y) {
                if (state.at(y).at(x) == CellState::Shown) {
                    _staticGrid.at(y).at(x) = CellState::Shown;
                }
            }
        }
    }

    void kill() {
        std::for_each(begin(_staticGrid) + 4, end(_staticGrid), [](Line& line) {
            bool hit = rstd::all_of(line, [](CellState cell) {
                    return cell == CellState::Shown;
        });
            if (hit) {
                rstd::fill(line, CellState::Dying);
            }
        });
    }

    void collect() {
        auto middle = rstd::stable_partition(_staticGrid, [](Line const& line) {
                return !rstd::all_of(line, [](CellState cell) {
                return cell == CellState::Dying;
            });
        });
        std::fill(middle, end(_staticGrid), Line(_hor, CellState::Hidden));
    }

    void drop() {
        collect();
        auto res = _dynamicGrid;
        std::copy(begin(res) + 1, end(res), begin(res));
        rstd::fill(res.at(_vert - 1), CellState::Hidden);

        if (collision(res)) {
            stamp(_dynamicGrid);
            _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
            _nothingFalling = true;
        } else {
            _dynamicGrid = res;
            _bbPiece.y -= 1;
        }
        kill();
    }

    State createState(int width, int height, CellState val) {
        State state;
        state.resize(height);
        for (Line& line : state) {
            PieceOrientation::t nextOrientation(PieceOrientation::t prev);
            line.resize(width);
            rstd::fill(line, val);
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
        return OrientationCircle[(prev + 1) % OrientationCircle.size()];
    }
public:
    impl(unsigned hor, unsigned vert)
        : _hor(hor + 8),
          _vert(vert + 8),
          _random(0, PieceType::count - 1)
    {
        State inner = createState(hor, vert + 4, CellState::Hidden);
        _staticGrid = createState(_hor, _vert, CellState::Shown);
        paste(inner, _staticGrid, 4, 4);
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
    }

    CellState getState(int x, int y) {
        if (_dynamicGrid.at(y + 4).at(x + 4) == CellState::Shown)
            return CellState::Shown;
        return _staticGrid.at(y + 4).at(x + 4);
    }

    void step() {
        if (_nothingFalling)
            nextPiece();
        drop();
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
        paste(pieces[_piece][newOrient], copy, _bbPiece.x + offset, _bbPiece.y);
        if (!collision(copy)) {
            _dynamicGrid = copy;
            _pieceOrientation = newOrient;
            _bbPiece.x += offset;
        }
    }
};

Tetris::Tetris(int hor, int vert)
    : _impl(new impl(hor, vert))
{ }

CellState Tetris::getState(int x, int y) {
    return _impl->getState(x, y);
}

void Tetris::step() {
    _impl->step();
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

Tetris::~Tetris() { }
