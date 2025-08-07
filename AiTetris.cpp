#include "AiTetris.h"
#include "simulator.h"
#include "Random.h"

#include <algorithm>

template <typename To, typename From>
To mapPiece(From aiPiece) {
    switch (aiPiece) {
        case From::J: return To::J;
        case From::L: return To::L;
        case From::S: return To::S;
        case From::Z: return To::Z;
        case From::T: return To::T;
        case From::I: return To::I;
        case From::O: return To::O;
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
        setPiece(move, info, CellState::Shown, mapPiece<PieceType::t, Piece::t>(move.piece));
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
    void eraseFallingPiece() override {}

    AiTetris(ITetris* source, int prefill) {
        assert(prefill < gBoardHeight);

        _curPiece = _rnd();
        _nextPiece = _rnd();
        _stats.level = 26;

        if (prefill != -1) {
            Random<int> rnd(0, 1);
            for (int r = 0; r < prefill; ++r) {
                for (int c = 0; c < gBoardWidth; ++c) {
                    if (rnd()) {
                        _sim.grid().set(19 - r, c);
                        _state[r][c].state = CellState::Shown;
                    }
                }
            }
            return;
        }

        if (!source || dynamic_cast<AiTetris const*>(source))
            return; // don't copy from another AiTetris

        source->eraseFallingPiece();
        auto sourceStats = source->getStats();
        _curPiece = mapPiece<Piece::t, PieceType::t>(sourceStats.piece);
        _nextPiece = mapPiece<Piece::t, PieceType::t>(sourceStats.nextPiece);
        _stats.level = sourceStats.level;

        for (int r = 0; r < gBoardHeight; ++r) {
            for (int c = 0; c < gBoardWidth; ++c) {
                if (source->getState(c, r).state == CellState::Shown) {
                    _sim.grid().set(19 - r, c);
                    _state[r][c].state = CellState::Shown;
                }
            }
        }
    }

    CellInfo getState(int x, int y) const override {
        return _state.at(y).at(x);
    }

    CellInfo getNextPieceState(int x, int y) const override {
        auto info = _sim.getPiece(_nextPiece, 0);
        return CellInfo((*info.grid)(3 - y, x) ? CellState::Shown
                                               : CellState::Hidden,
                        mapPiece<PieceType::t, Piece::t>(_nextPiece));
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

std::unique_ptr<ITetris> makeAiTetris(ITetris& source, int prefill) {
    return std::make_unique<AiTetris>(&source, prefill);
}
