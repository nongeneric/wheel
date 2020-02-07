#include <gtest/gtest.h>
#include "Tetris.h"
#include <functional>
#include <iostream>
#include <algorithm>
#include "vformat.h"
#include "rstd.h"
#include "HighscoreManager.h"
#include "Bitmap.h"

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

TEST(TetrisTests, Rotate) {
    Tetris t(TEST_DIMH, TEST_DIMV, []() { return PieceType::O; });
    t.rotate(true); // no piece is yet visible
    t.moveLeft();
    t.moveRight();
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

TEST(BitmapTest, RepresentationTest) {
    /*
     * coordinates:
     * [0;3] 6 7
     * [0;2] 4 5
     * [0;1] 2 3
     * [0;0] 0 1
     * memory:
     * 0 1 2 3 4 5 6 7
     */
    Bitmap b(2, 4, 8);
    ASSERT_EQ(2, b.pitch());
    b.fill(0);
    auto pos = b.data();
    std::all_of(pos, pos + b.pitch() * b.height(), [] (char x) { return x == 0; });

    b.setPixel(0, 0, 10);
    b.setPixel(0, 1, 10);
    ASSERT_EQ(10, b.pixel(0, 0));
    ASSERT_EQ(10, b.pixel(0, 1));
    ASSERT_EQ(10, pos[0]);
    ASSERT_EQ(10, pos[b.pitch()]);
}

TEST(BitmapTest, CreatingFromBytesTest) {
    // same representation
    char bytes[] = { 1, 2, 3, 4, 5, 6 };
    Bitmap same(bytes, 3, 2, 3, 8, false);
    ASSERT_EQ(1, same.pixel(0, 0));
    ASSERT_EQ(2, same.pixel(1, 0));
    ASSERT_EQ(3, same.pixel(2, 0));
    ASSERT_EQ(4, same.pixel(0, 1));
    ASSERT_EQ(5, same.pixel(1, 1));
    ASSERT_EQ(6, same.pixel(2, 1));

    // y-inverted representation
    char invertedbytes[] = { 4, 5, 6, 1, 2, 3 };
    Bitmap inverted(invertedbytes, 3, 2, 3, 8, true);
    ASSERT_EQ(1, inverted.pixel(0, 0));
    ASSERT_EQ(2, inverted.pixel(1, 0));
    ASSERT_EQ(3, inverted.pixel(2, 0));
    ASSERT_EQ(4, inverted.pixel(0, 1));
    ASSERT_EQ(5, inverted.pixel(1, 1));
    ASSERT_EQ(6, inverted.pixel(2, 1));
}

TEST(BitmapTest, CopyTest) {
    char bytes[] = {
        9,  10, 11, 12,
        5,  6,  7,  8,
        1,  2,  3,  4
    };
    Bitmap b(bytes, 4, 3, 4, 8, true);
    ASSERT_EQ(1, b.pixel(0, 0));
    ASSERT_EQ(6, b.pixel(1, 1));

    Bitmap copy = b.copy(1, 1, 2, 0);
    ASSERT_EQ(2, copy.width());
    ASSERT_EQ(2, copy.height());
    ASSERT_EQ(2, copy.pixel(0, 0));
    ASSERT_EQ(3, copy.pixel(1, 0));
    ASSERT_EQ(6, copy.pixel(0, 1));
    ASSERT_EQ(7, copy.pixel(1, 1));
}

TEST(BitmapTest, BlendPasteTest) {
    char bytes[] = {
        9,  10, 11, 12,
        5,  6,  7,  8,
        1,  2,  3,  4
    };
    Bitmap b(bytes, 4, 3, 4, 8, true);
    ASSERT_EQ(1, b.pixel(0, 0));
    ASSERT_EQ(6, b.pixel(1, 1));

    char srcbytes[] = {
        3, 4,
        1, 2
    };
    Bitmap src(srcbytes, 2, 2, 2, 8, true);
    b.blendPaste(src, 1, 1);

    ASSERT_EQ(1, b.pixel(0, 0));
    ASSERT_EQ(2, b.pixel(1, 0));
    ASSERT_EQ(3, b.pixel(2, 0));
    ASSERT_EQ(4, b.pixel(3, 0));

    ASSERT_EQ(5, b.pixel(0, 1));
    ASSERT_EQ(7, b.pixel(1, 1));
    ASSERT_EQ(9, b.pixel(2, 1));
    ASSERT_EQ(8, b.pixel(3, 1));

    ASSERT_EQ(9, b.pixel(0, 2));
    ASSERT_EQ(13, b.pixel(1, 2));
    ASSERT_EQ(15, b.pixel(2, 2));
    ASSERT_EQ(12, b.pixel(3, 2));
}

TEST(BitmapTest, pixels24bitsRGBAFormatTest) {
    Bitmap b(2, 2, 24);
    b.setPixel(0, 0, 0xAABBCC);
    b.setPixel(1, 0, 0x112233);
    ASSERT_EQ(0xAABBCC, b.pixel(0, 0));
    ASSERT_EQ(0x112233, b.pixel(1, 0));

    auto data = b.data();
    ASSERT_EQ(0xAAu, (unsigned char)data[0]);
    ASSERT_EQ(0xBBu, (unsigned char)data[1]);
    ASSERT_EQ(0xCCu, (unsigned char)data[2]);
}
