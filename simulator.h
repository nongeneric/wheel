#pragma once

#include <assert.h>
#include <stdint.h>

#include <array>
#include <bitset>
#include <cstring>
#include <optional>
#include <vector>

using Weights = std::array<float, 3>;

namespace Piece {
    enum t : uint8_t {
        J, L, S, Z, T, I, O, count
    };
}

template <int Rows, int RowOffset, int ColumnOffset>
struct PackedGridImpl {
    std::array<uint16_t, Rows> rows;

    void set(int r, int c) {
        rows[r + RowOffset] |= 1u << (16 - (c + ColumnOffset) - 1);
    }

    bool operator()(int r, int c) const {
        return rows[r + RowOffset] >> (16 - (c + ColumnOffset) - 1) & 1;
    }

    uint64_t toInt(int row) const {
        assert(0 <= row && row <= std::ssize(rows) - 4);
        uint64_t val;
        std::memcpy(&val, &rows[row], sizeof(val));
        return val;
    }

    void setInt(int row, uint64_t val) {
        assert(0 <= row && row <= std::ssize(rows) - 4);
        std::memcpy(&rows[row], &val, sizeof(val));
    }

    size_t size2() const { return 10; }
};

struct PackedPiece : PackedGridImpl<4, 0, 0> {
    PackedPiece() {
        rows = {};
    }

    size_t size1() const { return 4; }
};

constexpr int gFirstRow = 2;
constexpr int gLastRow = 21;
constexpr int gWallSize = 3;
constexpr int gBoardWidth = 10;
constexpr int gBoardHeight = 20;

/*
     0: xxx..........xxx < invisible
     1: xxx..........xxx < invisible
              ...
    21: xxx..........xxx
    22: xxxxxxxxxxxxxxxx < invisible
    23: xxxxxxxxxxxxxxxx < invisible
*/
struct PackedGrid : PackedGridImpl<24, 2, 3> {
    PackedGrid() {
        for (int i = 0; i <= 21; ++i)
            rows[i] = 0b1110000000000111;
        rows[22] = -1;
        rows[23] = -1;
    }

    bool operator==(PackedGrid const& other) const {
        return rows == other.rows;
    }

    size_t size1() const { return 20; }
};

struct Pos {
    Pos(char x, char y) : x(x), y(y) {}
    char x, y;
};

struct PieceInfo {
    Piece::t piece{};
    uint8_t rot = 0;
    PackedPiece const* grid = nullptr;
};

struct Move {
    Piece::t piece : 6;
    uint8_t rot : 2;
    uint8_t x;
    uint8_t y;

    uint32_t toInt() const {
        uint32_t res = 0;
        res |= piece;
        res <<= 2;
        res |= rot;
        res <<= 8;
        res |= x;
        res <<= 8;
        res |= y;
        return res;
    }

    void fromInt(uint32_t i) {
        y = i;
        i >>= 8;
        x = i;
        i >>= 8;
        rot = i;
        i >>= 2;
        piece = static_cast<Piece::t>(i);
    }
};

static_assert(sizeof(Move) == 3);

class Simulator {
    struct CellInfo {
        std::bitset<4> allowed{};
    };

    std::array<PackedPiece[4], Piece::count> _pieces;
    std::array<char, Piece::count> _pieceRots;
    std::array<std::array<CellInfo, 10>, 20> _cells;
    PackedGrid _grid;
    std::vector<Move> _moves;
    std::optional<Move> _bestMove;
    Weights _weights;

    int wrap(int i, int n);
    PieceInfo rotate(PieceInfo info, bool clockwise);
    void visit(Piece::t piece, Pos pos, int rot);
    float getQuality(PackedGrid const& board);
    float getQuality(std::optional<Piece::t> piece,
                     std::optional<Piece::t> nextPiece,
                     PackedGrid grid,
                     int level);

public:
    Simulator();

    bool analyze(Piece::t piece);
    std::optional<Move> getBestMove(Piece::t curPiece, std::optional<Piece::t> nextPiece);
    PackedGrid& grid();
    Weights& weights();
    void imprint(PackedGrid& grid, PieceInfo const& info, Pos pos);
    void erase(PackedGrid& grid, PieceInfo const& info, Pos pos);
    PieceInfo getPiece(Piece::t piece, uint8_t rot) const;
    std::vector<Move> interpolate(Move const& move);

    template <bool AllowClip>
    bool tryPlacing(Piece::t piece, int rot, Pos pos) {
        auto info = getPiece(piece, rot);
        auto pieceInt = info.grid->toInt(0) >> (pos.x + 1);
        auto gridInt = _grid.toInt(pos.y);
        if constexpr (!AllowClip) {
            if (pos.y < 3)
                gridInt |= (-1ull << 32) << (16 * pos.y);
        }
        return (pieceInt & gridInt) == 0;
    }
};

struct Heuristics {
    uint64_t filledTotal{};
    std::array<char, gBoardWidth> columnHeights{};

    explicit Heuristics(PackedGrid const& grid);
    float calcCompactness();
    float calcMaxHeight(PackedGrid const& grid);
    float calcDistortion();
};

inline std::string pieceNames = "JLSZTIO";

inline char getPieceName(Piece::t piece) {
    return pieceNames.at(piece);
};

inline Piece::t getPieceIdx(char name) {
    auto i = pieceNames.find(name);
    assert(i != std::string::npos);
    return static_cast<Piece::t>(i);
}

std::pair<PackedGrid, int> eliminate(PackedGrid const& grid);
