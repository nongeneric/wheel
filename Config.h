#pragma once

#include <string>

struct TetrisConfig {
    bool orthographic;
    bool fullScreen;
    unsigned screenWidth;
    unsigned screenHeight;
    bool showFps;
    unsigned initialLevel;
    void load(std::string const& fileName);
};
