#include "gtest/gtest.h"
#include "Tetris.h"
#include <functional>
#include <iostream>
#include "vformat.h"
#include "rstd.h"
#include "HighscoreManager.h"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

const int TEST_DIMH = 10;
const int TEST_DIMV = 5;

const auto _0 = CellState::Hidden;
const auto _1 = CellState::Shown;
const auto _2 = CellState::Dying;

std::string prettyPrint(std::function<CellState(int, int)> getter, int xMax, int yMax) {
    std::string res;
    for (int y = yMax - 1; y >= 0; --y) {
        res += "\n";
        for (int x = 0; x < xMax; ++x) {
            res += vformat("%d ", static_cast<int>(getter(x, y)));
        }
    }
    return res;
}

bool exact(Tetris const& tetris, std::vector<std::vector<CellState>> grid) {
    rstd::reverse(grid);
    for (int y = 0; y < TEST_DIMV; ++y) {
        for (int x = 0; x < TEST_DIMH; ++x) {
            if (tetris.getState(x, y).state != grid.at(y).at(x)) {
                std::cout << "was:";
                std::cout << prettyPrint([&](int x, int y) {
                    return tetris.getState(x, y).state;
                }, TEST_DIMH, TEST_DIMV);
                std::cout << "expected:";
                std::cout << prettyPrint([&](int x, int y) {
                    return grid.at(y).at(x);
                }, TEST_DIMH, TEST_DIMV);
                return false;
            }
        }
    }
    return true;
}

TEST(TetrisTests, Simple1) {
    Tetris t(TEST_DIMH, TEST_DIMV, []() { return PieceType::O; });
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.moveRight();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _1, _1, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _1, _1, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.moveRight();
    t.moveRight();
    t.moveRight();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.moveRight(); // must not move outside the border
    t.moveRight();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
    t.step();
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
    }));
    t.moveLeft(); // must move not-yet-frozen piece
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _1, _1, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _1, _1, _0 },
    }));
    t.moveRight();
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
    })); // the piece is frozen now, another one isn't falling just yet
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _1, _1 },
    })); // another one starts falling
    t.moveRight();
    t.moveRight();
    t.step();
    t.step();
    t.step();
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _1, _1, _1, _1 },
        { _0, _0, _0, _0, _0, _0, _1, _1, _1, _1 },
    })); // both are frozen
    // place another two pieces
    t.step();
    t.moveLeft();
    t.moveLeft();
    t.moveLeft();
    t.moveLeft();
    t.step();
    t.step();
    t.step();
    t.step();
    t.step(); // new piece appears
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _1, _1, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _1, _1, _0, _0, _0, _0, _1, _1, _1, _1 },
        { _1, _1, _0, _0, _0, _0, _1, _1, _1, _1 },
    }));
    t.moveLeft();
    t.moveLeft();
    t.step();
    t.step();
    t.step();
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _1, _1, _1, _1, _0, _0, _1, _1, _1, _1 },
        { _1, _1, _1, _1, _0, _0, _1, _1, _1, _1 },
    }));
    t.step();
    t.step();
    t.step();
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _1, _1, _1, _1, _1, _1, _1, _1, _1, _1 },
        { _1, _1, _1, _1, _1, _1, _1, _1, _1, _1 },
    })); // two lines are complete, but not frozen
    t.step();
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _2, _2, _2, _2, _2, _2, _2, _2, _2, _2 },
        { _2, _2, _2, _2, _2, _2, _2, _2, _2, _2 },
    }));
    t.collect(); // should be called to remove dying pieces
    ASSERT_TRUE(exact(t, {
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
        { _0, _0, _0, _0, _0, _0, _0, _0, _0, _0 },
    }));
}


TEST(HighscoresTest, Simple1) {
    std::vector<HighscoreRecord> records;
    HighscoreRecord r { "test", 15, 2000, 10 };
    ASSERT_TRUE(r == r);

    int pos = updateHighscores(r, records, true);
    ASSERT_EQ(0, pos);
    ASSERT_EQ(1, records.size());
    ASSERT_TRUE(r == records.front());

    pos = updateHighscores(r, records, true);
    ASSERT_TRUE(pos == 0 || pos == 1);
    ASSERT_EQ(2, records.size());
    ASSERT_TRUE(r == records.front());
    ASSERT_TRUE(r == records.back());

    HighscoreRecord r1 { "test1", 15, 2000, 19 };
    pos = updateHighscores(r1, records, true);
    ASSERT_EQ(0, pos);
    ASSERT_EQ(3, records.size());
    ASSERT_TRUE(r1 == records.front());

    HighscoreRecord r2 { "test2", 16, 2000, 19 };
    pos = updateHighscores(r2, records, true);
    ASSERT_EQ(0, pos);
    ASSERT_EQ(4, records.size());
    ASSERT_TRUE(r2 == records.front());

    HighscoreRecord r3 { "test2", 10, 2500, 19 };
    pos = updateHighscores(r3, records, false);
    ASSERT_EQ(0, pos);
    ASSERT_EQ(5, records.size());
    ASSERT_TRUE(r3 == records.front());

    for (int i = 0; i < 10; ++i) {
        updateHighscores(r3, records, false);
    }

    for (auto hs : records) {
        ASSERT_TRUE(hs == r3);
    }

    HighscoreRecord r4 { "test2", 10, 2400, 19 };

    ASSERT_EQ(6, records.size());
    records.push_back(r4);
    pos = updateHighscores(r4, records, true);
    ASSERT_EQ(-1, pos);
    ASSERT_EQ(6, records.size());

    pos = updateHighscores({ "", 100, 0, 0 }, records, true);
    ASSERT_EQ(0, pos);
    pos = updateHighscores({ "", 105, 0, 0 }, records, true);
    ASSERT_EQ(0, pos);
    pos = updateHighscores({ "", 110, 0, 0 }, records, true);
    ASSERT_EQ(0, pos);
    pos = updateHighscores({ "", 101, 0, 0 }, records, true);
    ASSERT_EQ(2, pos);

    HighscoreRecord r0 { "", 0, 0, 0 };
    records.insert(begin(records), r0);
    pos = updateHighscores(r0, records, true);
    ASSERT_EQ(-1, pos);
    ASSERT_FALSE(records.front() == r0);
}
