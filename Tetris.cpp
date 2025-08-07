#include "Tetris.h"
#include <assert.h>
#include "Random.h"

#include <vector>
#include <functional>
#include <algorithm>

struct BBox {
    int x = 0;
    int y = 0;
    int size = 0;
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

class Tetris : public ITetris {
    int _hor, _vert;
    bool _nothingFalling;
    State _staticGrid;
    State _dynamicGrid;
    BBox _bbPiece;
    PieceType::t _piece;
    PieceType::t _nextPiece;
    PieceOrientation::t _pieceOrientation;
    std::function<PieceType::t()> _generator;
    TetrisStatistics _stats;
    unsigned _initialLevel;

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

    void paste(State const& piece, State& state, unsigned posX, unsigned posY) const {
        for (size_t y = 0; y < piece.size(); ++y) {
            std::ranges::copy(piece.at(y), begin(state.at(posY + y)) + posX);
        }
    }

    State patchPiece(State const& piece, PieceType::t pieceType) const {
        State copy = piece;
        std::ranges::reverse(copy); // pieces and the grid have different coord systems (y axis)
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
        _nextPiece = _generator();
        State piece = patchPiece(pieces[_piece][_pieceOrientation], _piece);
        int yoffset = pieceInitialYOffset[_piece];
        int xoffset = 4 + (_hor - 8) / 2 - piece.size() / 2;
        _bbPiece = { xoffset, _vert - 4 - (int)piece.size() + yoffset, (int)piece.size() };
        State copy = _dynamicGrid;
        paste(piece, copy, _bbPiece.x, _bbPiece.y);
        if (collision(copy)) {
            _stats.gameOver = true;
        } else {
            _dynamicGrid = copy;
        }
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
            bool hit = std::ranges::all_of(line, [](CellInfo cell) {
                    return cell.state == CellState::Shown;
            });
            if (hit) {
                std::ranges::fill(line, CellInfo { CellState::Dying });
            }
        });
    }

    void updateStats(int lines) {
        _stats.lines += lines;
        int scores[] = { 0, 100, 200, 400, 800 };
        _stats.score += scores[lines];
        _stats.level = std::max(_stats.lines / 10, _initialLevel);
    }

    void drop() {
        auto res = _dynamicGrid;
        std::copy(begin(res) + 1, end(res), begin(res));
        std::ranges::fill(res.at(_vert - 1), CellInfo { CellState::Hidden });

        if (collision(res)) {
            stamp(_dynamicGrid);
            _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
            _nothingFalling = true;
        } else {
            _dynamicGrid = res;
            _bbPiece.y -= 1;
        }
    }

    State createState(int width, int height, CellState val) const {
        State state;
        state.resize(height);
        for (Line& line : state) {
            line.resize(width);
            std::ranges::fill(line, CellInfo { val });
        }
        return state;
    }

    void movePieceHor(State& state, int offset) {
        auto box = cut(state, _bbPiece.x, _bbPiece.y, _bbPiece.size);
        paste(box, state, _bbPiece.x + offset, _bbPiece.y);
    }

    void moveHor(int offset) {
        if (_nothingFalling || _stats.gameOver)
            return;
        auto state = _dynamicGrid;
        movePieceHor(state, offset);
        if (!collision(state)) {
            _dynamicGrid = state;
            _bbPiece.x += offset;
        }
    }

    PieceOrientation::t nextOrientation(PieceOrientation::t prev, bool clockwise) {
        int delta = clockwise ? 1 : -1;
        return static_cast<PieceOrientation::t>((prev + delta + PieceOrientation::count) % PieceOrientation::count);
    }
public:
    Tetris(int hor, int vert, std::function<PieceType::t()> generator)
        : _hor(hor + 8), _vert(vert + 8), _generator(generator), _initialLevel(0)
    {
        _nothingFalling = true;
        _nextPiece = _generator();
        _piece = _nextPiece;
        State inner = createState(_hor - 8, _vert - 4, CellState::Hidden);
        _staticGrid = createState(_hor, _vert, CellState::Shown);
        paste(inner, _staticGrid, 4, 4);
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
        _stats = TetrisStatistics();
    }

    int collect() override {
        auto middle = std::ranges::stable_partition(_staticGrid, [](Line const& line) {
            return !std::ranges::all_of(line, [](CellInfo cell) {
                return cell.state == CellState::Dying;
            });
        });
        auto lines = middle.size();
        updateStats(lines);
        Line emptyLine(_hor, { CellState::Shown });
        std::fill(begin(emptyLine) + 4, end(emptyLine) - 4, CellInfo { CellState::Hidden });
        std::ranges::fill(middle, emptyLine);
        return lines;
    }

    CellInfo getState(int x, int y) const override {
        CellInfo const& dynInfo = _dynamicGrid.at(y + 4).at(x + 4);
        if (dynInfo.state == CellState::Shown)
            return dynInfo;
        return _staticGrid.at(y + 4).at(x + 4);
    }

    bool step() override {
        if (_nothingFalling) {
            nextPiece();
            kill();
            return true;
        }
        drop();
        kill();
        return false;
    }

    void moveRight() override {
        moveHor(1);
    }

    void moveLeft() override {
        moveHor(-1);
    }

    void rotate(bool clockwise) override {
        if (_bbPiece.y < 0)
            return;
        auto copy = _dynamicGrid;
        int rightSpike = std::max(0, _bbPiece.x + _bbPiece.size - (_hor - 4));
        int leftSpike = std::max(0, 4 - _bbPiece.x);
        int offset = leftSpike - rightSpike;
        movePieceHor(copy, offset);
        auto newOrient = nextOrientation(_pieceOrientation, clockwise);
        State nextPiece = patchPiece(pieces[_piece][newOrient], _piece);
        paste(nextPiece, copy, _bbPiece.x + offset, _bbPiece.y);
        if (!collision(copy)) {
            _dynamicGrid = copy;
            _pieceOrientation = newOrient;
            _bbPiece.x += offset;
        }
    }

    TetrisStatistics getStats() const override {
        TetrisStatistics stats = _stats;
        stats.piece = _piece;
        stats.nextPiece = _nextPiece;
        return stats;
    }

    CellInfo getNextPieceState(int x, int y) const override {
        State state = createState(4, 4, CellState::Hidden);
        assert((unsigned)_nextPiece < PieceType::count);
        State piece = patchPiece(pieces[_nextPiece][PieceOrientation::Up], _nextPiece);
        paste(piece, state, 0, 0);
        assert((unsigned)state.at(y).at(x).piece < PieceType::count);
        return state.at(y).at(x);
    }

    void setInitialLevel(int level) override {
        _initialLevel = level;
    }

    void eraseFallingPiece() override {
        _dynamicGrid = createState(_hor, _vert, CellState::Hidden);
    }
};

std::unique_ptr<ITetris> makeTetris(int hor, int vert, std::function<PieceType::t()> generator) {
    return std::make_unique<Tetris>(hor, vert, generator);
}
