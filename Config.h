#pragma once

#include <string>
#include <vector>

struct HighscoreRecord {
    std::string name;
    int lines;
    int score;
    int initialLevel;
};

struct TetrisConfig {
    bool orthographic;
    bool fullScreen;
    unsigned screenWidth;
    unsigned screenHeight;
    bool showFps;
    unsigned initialLevel;
    std::vector<HighscoreRecord> highscoreLines;
    std::vector<HighscoreRecord> highscoreScore;
    void load(std::string const& fileName);
    void save(std::string const& fileName);
};
