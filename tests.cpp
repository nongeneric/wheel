#include "gtest/gtest.h"
#include "Tetris.h"
#include <functional>
#include "vformat.h"
#include <boost/log/trivial.hpp>
#include "rstd.h"

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
                BOOST_LOG_TRIVIAL(error) << "was:";
                BOOST_LOG_TRIVIAL(error) << prettyPrint([&](int x, int y) {
                    return tetris.getState(x, y).state;
                }, TEST_DIMH, TEST_DIMV);
                BOOST_LOG_TRIVIAL(error) << "expected:";
                BOOST_LOG_TRIVIAL(error) << prettyPrint([&](int x, int y) {
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
