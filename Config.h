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
    X(MainMenu_Restart) \
    X(MainMenu_Options) \
    X(MainMenu_HallOfFame) \
    X(MainMenu_Exit) \
    X(OptionsMenu_Back) \
    X(OptionsMenu_Resolution) \
    X(OptionsMenu_Fullscreen) \
    X(OptionsMenu_InitialLevel) \
    X(HUD_Lines) \
    X(HUD_Score) \
    X(HUD_Level) \
    X(HUD_FPS) \
    X(GameOverScreen_NewHighscore) \
    X(GameOverScreen_GameOver) \
    X(GameOverScreen_PressEnter) \
    X(GameOverScreen_EnterYourName) \
    X(HallOfFame_Name) \
    X(HallOfFame_Lines) \
    X(HallOfFame_Score) \
    X(HallOfFame_Level)

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
