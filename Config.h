#pragma once

#include <string>
#include <vector>
#include <map>

struct HighscoreRecord {
    std::string name;
    unsigned lines;
    unsigned score;
    unsigned initialLevel;
    bool operator==(HighscoreRecord const&) const;
};

#define STRING_ID_LIST \
    X(MainMenu_Resume) \
    X(MainMenu_Restart) \
    X(MainMenu_StartAi) \
    X(MainMenu_Options) \
    X(MainMenu_HallOfFame) \
    X(MainMenu_Exit) \
    X(Menu_Yes) \
    X(Menu_No) \
    X(OptionsMenu_Back) \
    X(OptionsMenu_Display) \
    X(OptionsMenu_Resolution) \
    X(OptionsMenu_DisplayMode) \
    X(OptionsMenu_DisplayMode_Fullscreen) \
    X(OptionsMenu_DisplayMode_Windowed) \
    X(OptionsMenu_DisplayMode_Borderless) \
    X(OptionsMenu_InitialLevel) \
    X(OptionsMenu_AiPrefill) \
    X(OptionsMenu_CopyPrefill) \
    X(OptionsMenu_Rumble) \
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

enum class DisplayMode {
    Fullscreen,
    Windowed,
    Borderless
};

struct TetrisConfig {
    bool orthographic;
    DisplayMode displayMode;
    unsigned monitor;
    unsigned screenWidth;
    unsigned screenHeight;
    bool showFps;
    unsigned initialLevel;
    int aiPrefill;
    bool rumble;
    int fpsCap;
    std::string language;
    std::vector<HighscoreRecord> highscoreLines;
    std::vector<HighscoreRecord> highscoreScore;
    std::string string(StringID id) const;
    void load();
    void save();
private:
    void loadStrings();
    std::map<StringID, std::string> _strings;
};

std::string printDisplayMode(DisplayMode mode);
