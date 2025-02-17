#include "simulator.h"

#include <set>
#include <deque>
#include <numeric>
#include <map>
#include <unordered_map>
#include <algorithm>

#include <immintrin.h>

using namespace std::string_literals;

std::pair<PackedGrid, int> eliminate(PackedGrid const& grid) {
    auto res = grid;
    int destRow = gLastRow;
    int lines = 0;
    for (auto r = gLastRow; r >= gFirstRow; --r) {
        if (grid.rows[r] != uint16_t(-1)) {
            res.rows[destRow--] = grid.rows[r];
        } else {
            lines++;
        }
    }
    return {res, lines};
}

PieceInfo Simulator::rotate(PieceInfo info, bool clockwise) {
    int delta = clockwise ? 1 : -1;
    info.rot = wrap(info.rot + delta, _pieceRots[info.piece]);
    info.grid = &_pieces[info.piece][info.rot];
    return info;
}

int Simulator::wrap(int i, int n) {
    return ((i % n) + n) % n;
}

void Simulator::visit(Piece::t piece, Pos pos, int rot) {
    if (!tryPlacing<true>(piece, rot, pos))
        return;
    auto& cell = _cells.at(pos.y).at(pos.x);
    // already visited from another side
    if (cell.allowed[rot])
        return;
    cell.allowed[rot] = true;
    switch (_pieceRots[piece]) {
        case 1: cell.allowed[0] = true; break;
        case 2: {
            int other = wrap(rot + 1, 2);
            cell.allowed[other] = tryPlacing<true>(piece, other, pos);
            break;
        }
        case 4: {
            int right = wrap(rot + 1, 4);
            int left = wrap(rot - 1, 4);
            if (!cell.allowed[right])
                cell.allowed[right] = tryPlacing<true>(piece, right, pos);
            if (!cell.allowed[left])
                cell.allowed[left] = tryPlacing<true>(piece, left, pos);
            int last = wrap(right + 1, 4);
            if (!cell.allowed[last] && (cell.allowed[right] || cell.allowed[left]))
                cell.allowed[last] = tryPlacing<true>(piece, last, pos);
            break;
        }
    }
    for (int r = 0; r < _pieceRots[piece]; ++r) {
        if (cell.allowed[r]) {
            visit(piece, {char(pos.x - 1), pos.y}, r);
            visit(piece, {char(pos.x + 1), pos.y}, r);
            visit(piece, {pos.x, char(pos.y + 1)}, r);

            if (pos.y == gBoardHeight - 1 || (pos.y < gBoardHeight - 1 && !_cells.at(pos.y + 1).at(pos.x).allowed[r])) {
                if (tryPlacing<false>(piece, r, pos))
                    _moves.emplace_back(piece, r, pos.x, pos.y);
            }
        }
    }
}

Simulator::Simulator() {
    _weights = {0.33333334, 0.5833333, 0.183333336};
    _pieceRots = {4, 4, 2, 2, 4, 2, 1};

    _pieces[0][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0111 << 12,
        0b0001 << 12
    };
    _pieces[0][1].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0010 << 12,
        0b0110 << 12
    };
    _pieces[0][2].rows = {
        0b0000 << 12,
        0b0100 << 12,
        0b0111 << 12,
        0b0000 << 12
    };
    _pieces[0][3].rows = {
        0b0000 << 12,
        0b0011 << 12,
        0b0010 << 12,
        0b0010 << 12
    };

    _pieces[1][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0111 << 12,
        0b0100 << 12
    };
    _pieces[1][1].rows = {
        0b0000 << 12,
        0b0110 << 12,
        0b0010 << 12,
        0b0010 << 12
    };
    _pieces[1][2].rows = {
        0b0000 << 12,
        0b0001 << 12,
        0b0111 << 12,
        0b0000 << 12
    };
    _pieces[1][3].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0010 << 12,
        0b0011 << 12
    };

    _pieces[2][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0011 << 12,
        0b0110 << 12
    };
    _pieces[2][1].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0011 << 12,
        0b0001 << 12
    };

    _pieces[3][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0110 << 12,
        0b0011 << 12
    };
    _pieces[3][1].rows = {
        0b0000 << 12,
        0b0001 << 12,
        0b0011 << 12,
        0b0010 << 12
    };

    _pieces[4][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0111 << 12,
        0b0010 << 12
    };
    _pieces[4][1].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0110 << 12,
        0b0010 << 12
    };
    _pieces[4][2].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0111 << 12,
        0b0000 << 12
    };
    _pieces[4][3].rows = {
        0b0000 << 12,
        0b0010 << 12,
        0b0011 << 12,
        0b0010 << 12
    };

    _pieces[5][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b1111 << 12,
        0b0000 << 12
    };
    _pieces[5][1].rows = {
        0b0010 << 12,
        0b0010 << 12,
        0b0010 << 12,
        0b0010 << 12
    };

    _pieces[6][0].rows = {
        0b0000 << 12,
        0b0000 << 12,
        0b0110 << 12,
        0b0110 << 12
    };
}

bool Simulator::analyze(Piece::t piece) {
    for (auto& r : _cells) {
        for (auto& c : r) {
            c = {};
        }
    }
    _moves.clear();
    visit(piece, Pos(5, 0), 0);
    return _cells[0][5].allowed[0];
}

float Simulator::getQuality(PackedGrid const& board) {
    if (board(0, 5))
        return 0;
    Heuristics hs(board);
    auto maxHeight = hs.calcMaxHeight(board);
    auto compactness = hs.calcCompactness();
    auto distortion = hs.calcDistortion();
    auto vals = std::array{maxHeight, compactness, distortion};
    float quality = 0;
    assert(vals.size() == _weights.size());
    for (size_t i = 0; i < vals.size(); ++i) {
        quality += vals[i] * _weights[i];
    }
    return quality;
}

float Simulator::getQuality(std::optional<Piece::t> piece,
                            std::optional<Piece::t> nextPiece,
                            PackedGrid grid,
                            int level) {
    if (level == 3)
        return getQuality(grid);
    std::string pieces;
    if (piece.has_value()) {
        pieces.append(1, piece.value());
    } else {
        pieces = "\0\1\2\3\4\5\6"s;
    }
    float const probability = 1. / pieces.size();
    float resQ = 0;
    for (char p : pieces) {
        float q = 0;
        _grid = grid;
        if (!analyze(static_cast<Piece::t>(p)))
            continue;
        auto moves = std::move(_moves);
        for (auto m : moves) {
            imprint(grid, getPiece(m.piece, m.rot), {(char)m.x, (char)m.y});
            auto elimGrid = eliminate(grid).first;
            erase(grid, getPiece(m.piece, m.rot), {(char)m.x, (char)m.y});
            auto childQ = getQuality(nextPiece, std::nullopt, elimGrid, level + 1);
            if (q < childQ) {
                q = childQ;
                if (level == 0)
                    _bestMove = m;
            }
        }
        resQ += probability * q;
    }
    return resQ;
}

std::optional<Move> Simulator::getBestMove(Piece::t curPiece,
                                           std::optional<Piece::t> nextPiece) {
    auto copy = _grid;
    _bestMove.reset();
    getQuality(curPiece, nextPiece, _grid, 0);
    _grid = copy;
    return _bestMove;
}

PackedGrid& Simulator::grid() {
    return _grid;
}

Weights& Simulator::weights() {
    return _weights;
}

void Simulator::imprint(PackedGrid& grid, PieceInfo const& info, Pos pos) {
    assert([&] {
        auto copy = grid;
        erase(grid, info, pos);
        return copy == grid;
    }());
    grid.setInt(pos.y, grid.toInt(pos.y) | (info.grid->toInt(0) >> (pos.x + 1)));
}

void Simulator::erase(PackedGrid& grid, PieceInfo const& info, Pos pos) {
    uint64_t u64grid = grid.toInt(pos.y);
    uint64_t u64piece = info.grid->toInt(0) >> (pos.x + 1);
    u64grid &= ~u64piece | 0xe007e007e007e007ull;
    grid.setInt(pos.y, u64grid);
}

PieceInfo Simulator::getPiece(Piece::t piece, uint8_t rot) const {
    assert(rot < _pieceRots[piece]);
    return {piece, rot, &_pieces[piece][rot]};
}

struct PiecePlacement {
    uint8_t r = -1;
    uint8_t c = -1;
    uint8_t rot = -1;
    int distance = std::numeric_limits<int>::max();
    PiecePlacement* source = nullptr;
};

struct Edge {
    PiecePlacement* pp = nullptr;
    int distance = -1;
};

std::vector<Move> Simulator::interpolate(Move const& move) {
    std::map<std::tuple<uint8_t, uint8_t, uint8_t>, PiecePlacement*> ppmap;
    std::deque<PiecePlacement> vertices;
    std::unordered_map<PiecePlacement*, std::vector<Edge>> edges;

    for (uint8_t r = 0; r < gBoardHeight; ++r) {
        for (uint8_t c = 0; c < gBoardWidth; ++c) {
            for (uint8_t rot = 0; rot < 4; ++rot) {
                if (!_cells[r][c].allowed[rot])
                    continue;
                vertices.push_back({r, c, rot});
                ppmap[{r, c, rot}] = &vertices.back();
            }
        }
    }
    // connect adjacent rots inside a single cell
    for (int r = 0; r < gBoardHeight; ++r) {
        for (int c = 0; c < gBoardWidth; ++c) {
            int rotNum = _pieceRots[move.piece];
            for (int rot = 0; rot < rotNum; ++rot) {
                int nextRot = wrap(rot + 1, rotNum);
                if (_cells[r][c].allowed[rot] && _cells[r][c].allowed[nextRot]) {
                    auto first = ppmap.at({r, c, rot});
                    auto second = ppmap.at({r, c, nextRot});
                    edges[first].push_back({second, 1});
                    edges[second].push_back({first, 1});
                }
            }
        }
    }
    for (auto& pp : vertices) {
        if (pp.c > 0 && _cells[pp.r][pp.c - 1].allowed[pp.rot]) { // left
            edges[&pp].push_back({ppmap.at({pp.r, pp.c - 1, pp.rot}), 1});
        }
        if (pp.c < gBoardWidth - 1 && _cells[pp.r][pp.c + 1].allowed[pp.rot]) { // right
            edges[&pp].push_back({ppmap.at({pp.r, pp.c + 1, pp.rot}), 1});
        }
        if (pp.r < gBoardHeight - 1 && _cells[pp.r + 1][pp.c].allowed[pp.rot]) { // down
            edges[&pp].push_back({ppmap.at({pp.r + 1, pp.c, pp.rot}), 1});
        }
    }

    auto comp = [](PiecePlacement const* a, PiecePlacement const* b) {
        return a->distance < b->distance;
    };
    using QType = std::multiset<PiecePlacement*, decltype(comp)>;
    QType q(comp);

    ppmap.at({0, 5, 0})->distance = 0; // starting piece position
    for (auto& pp : vertices) {
        q.insert(&pp);
    }
    std::unordered_map<PiecePlacement*, QType::iterator> qiterMap;
    for (auto it = q.begin(); it != q.end(); ++it) {
        qiterMap[*it] = it;
    }
    while (!q.empty()) {
        auto* minpp = *q.begin();
        q.erase(q.begin());
        for (auto edge : edges[minpp]) {
            if (edge.pp->distance > minpp->distance + edge.distance) {
                edge.pp->distance = minpp->distance + edge.distance;
                q.erase(qiterMap.at(edge.pp));
                edge.pp->source = minpp;
                q.insert(edge.pp);
            }
        }
    }

    std::vector<Move> moves;
    auto dest = ppmap.at({move.y, move.x, move.rot});
    while (dest) {
        moves.push_back(
            Move{.piece = move.piece, .rot = dest->rot, .x = dest->c, .y = dest->r});
        dest = dest->source;
    }
    std::ranges::reverse(moves);
    return moves;
}

Heuristics::Heuristics(PackedGrid const& grid) {
    uint64_t mask = 0;
    uint64_t columnTotals = 0;
    for (int r = gFirstRow; r <= gLastRow; ++r) {
        auto row = grid.rows[r];
        mask |= row;
        columnTotals += _pdep_u64(mask >> 3, 0x210842108421ull);
    }

    for (int i = gBoardWidth - 1; i >= 0; --i) {
        columnHeights[i] = columnTotals & 0b11111;
        columnTotals >>= 5;
    }

    for (int r = gFirstRow; r <= gLastRow; r += 4) {
        filledTotal += _mm_popcnt_u64(grid.toInt(r));
    }
    filledTotal -= gWallSize * 2 * gBoardHeight;
}

float Heuristics::calcCompactness() {
    int total = 0;
    for (char h : columnHeights) {
        total += h;
    }
    if (total == 0)
        return 1;
    return float(filledTotal) / total;
}

float Heuristics::calcMaxHeight(PackedGrid const& grid) {
    for (int r = gFirstRow; r <= gLastRow; ++r) {
        if (grid.rows[r] != 0xe007)
            return (r - 2) / 20.;
    }
    return 1.;
}

float Heuristics::calcDistortion() {
    int diffs = 0;
    for (int c = 1; c < gBoardWidth; ++c) {
        diffs += std::abs(columnHeights[c] - columnHeights[c - 1]);
    }
    auto const maxDiffs = 20 * 9;
    return 1 - float(diffs) / maxDiffs;
}
