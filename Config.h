#pragma once

#include <string>
#include <vector>
#include <map>

struct HighscoreRecord {
    std::string name;
    unsigned lines;
    unsigned score;
    unsigned initialLevel;
    bool operator==(HighscoreRecord const&);
};

#define STRING_ID_LIST \
    X(MainMenu_Resume) \
    X(MainMenu_Exit)

#define X(s) s,
enum class StringID {
    STRING_ID_LIST
};
#undef X

struct TetrisConfig {    
    bool orthographic;
    bool fullScreen;
    unsigned screenWidth;
    unsigned screenHeight;
    bool showFps;
    unsigned initialLevel;
    std::string language;
    std::vector<HighscoreRecord> highscoreLines;
    std::vector<HighscoreRecord> highscoreScore;
    std::string string(StringID id);
    void load();
    void save();
private:
    void loadStrings();
    std::map<StringID, std::string> _strings;
};
