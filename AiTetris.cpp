#include "AiTetris.h"
#include "simulator.h"
#include "Random.h"

#include <algorithm>

PieceType::t mapPiece(Piece::t aiPiece) {
    switch (aiPiece) {
        case Piece::J: return PieceType::J;
        case Piece::L: return PieceType::L;
        case Piece::S: return PieceType::S;
        case Piece::Z: return PieceType::Z;
        case Piece::T: return PieceType::T;
        case Piece::I: return PieceType::I;
        case Piece::O: return PieceType::O;
        default: assert(false); return {};
    }
}

class AiTetris : public ITetris {
    Simulator _sim;
    Piece::t _curPiece{};
    Piece::t _nextPiece{};
    std::array<std::array<CellInfo, gBoardWidth>, gBoardHeight> _state{};
    std::vector<Move> _moves;
    size_t _curMove = 0;
    Random<Piece::t> _rnd{Piece::t{}, Piece::t(Piece::count - 1)};
    TetrisStatistics _stats;

    void setPiece(Move move, PieceInfo info, CellState state, PieceType::t piece = PieceType::O) {
        for (int r = 0; r < 4; ++r) {
            for (int c = 0; c < 4; ++c) {
                auto gpos = Pos(move.x - 2 + c, 19 - (move.y - 2 + r));
                if ((*info.grid)(r, c) && 0 <= gpos.x && gpos.x < 10 &&
                    0 <= gpos.y && gpos.y < 20) {
                    auto& cell = _state.at(gpos.y).at(gpos.x);
                    assert((state == CellState::Shown) ^ (cell.state == CellState::Shown));
                    cell.state = state;
                    cell.piece = piece;
                }
            }
        }
    }

    void updateState() {
        if (_curMove > 0) {
            auto move = _moves.at(_curMove - 1);
            auto info = _sim.getPiece(move.piece, move.rot);
            setPiece(move, info, CellState::Hidden);
        }
        auto move = _moves.at(_curMove);
        auto info = _sim.getPiece(move.piece, move.rot);
        setPiece(move, info, CellState::Shown, mapPiece(move.piece));
        if (_curMove == _moves.size() - 1) {
            for (auto& r : _state) {
                bool isCompleteLine = std::ranges::all_of(r, [](CellInfo const& info) {
                    return info.state == CellState::Shown;
                });
                if (isCompleteLine) {
                    std::ranges::fill(r, CellState::Dying);
                }
            }
        }
    }


public:
    void setInitialLevel(int /*level*/) override {}
    void moveRight() override {
        _stats.level = std::min(_stats.level + 1, 35u);
    }
    void moveLeft() override {
        _stats.level = std::max(_stats.level - 1, 1u);
    }
    void rotate(bool /*clockwise*/) override {}
    void reset() override {}
    void resetGameOver() override {}

    AiTetris() {
        _curPiece = _rnd();
        _nextPiece = _rnd();
        _stats.level = 26;
    }

    CellInfo getState(int x, int y) const override {
        return _state.at(y).at(x);
    }

    CellInfo getNextPieceState(int x, int y) const override {
        auto info = _sim.getPiece(_nextPiece, 0);
        return CellInfo((*info.grid)(3 - y, x) ? CellState::Shown
                                               : CellState::Hidden,
                        mapPiece(_nextPiece));
    }

    bool step() override {
        if (!_moves.empty()) {
            updateState();
            _curMove++;
        }
        if (_curMove == _moves.size()) {
            if (!_moves.empty()) {
                auto move = _moves.back();
                _sim.imprint(_sim.grid(),
                             _sim.getPiece(move.piece, move.rot),
                             {char(move.x), char(move.y)});
                auto const& [grid, lines] = eliminate(_sim.grid());
                _sim.grid() = grid;
                _stats.lines += lines;
                _curPiece = _nextPiece;
                _nextPiece = _rnd();
            }

            auto move = _sim.getBestMove(_curPiece, _nextPiece);
            if (!move.has_value()) {
                _stats.gameOver = true;
                return false;
            }
            _sim.analyze(move->piece);
            _moves = _sim.interpolate(*move);
            _moves.push_back((_moves.back()));
            _curMove = 0;
        }
        return _curMove == 0;
    }

    int collect() override {
        int lines = 0;
        int dstR = 0;
        for (int srcR = 0; srcR < 20; ++srcR) {
            bool isCompleteLine = std::ranges::all_of(_state.at(srcR), [](CellInfo const& info) {
                return info.state == CellState::Dying;
            });
            if (!isCompleteLine) {
                _state.at(dstR++) = _state.at(srcR);
            } else {
                lines++;
            }
        }
        return lines;
    }

    TetrisStatistics getStats() const override {
        return _stats;
    }
};

std::unique_ptr<ITetris> makeAiTetris() {
    return std::make_unique<AiTetris>();
}
