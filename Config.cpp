#include "Config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

/*
<tetris orthographic="true"
        fullscreen="true"
        showFps="false"
        initialLevel="19">
    <resolution width="1920"
                height="1080"/>
</tetris>
*/

using boost::property_tree::ptree;
const std::string configName = "config.xml";

void readHighscores(std::vector<HighscoreRecord>& vec, std::string path, ptree& pt) {
    auto child = pt.get_child_optional(path);
    if (!child)
        return;
    for (auto& node : child.get()) {
        vec.push_back(HighscoreRecord{
            node.second.get("<xmlattr>.name", ""),
            node.second.get("<xmlattr>.lines", 0u),
            node.second.get("<xmlattr>.score", 0u),
            node.second.get("<xmlattr>.level", 0u)
        });
    }
}

void writeHighscores(std::vector<HighscoreRecord> const& vec, std::string path, ptree& pt) {
    for (auto& record : vec) {
        ptree child;
        child.put("<xmlattr>.name", record.name);
        child.put("<xmlattr>.lines", record.lines);
        child.put("<xmlattr>.score", record.score);
        child.put("<xmlattr>.level", record.initialLevel);
        pt.add_child(path, child);
    }
}

std::string TetrisConfig::string(StringID id) {
    auto it = _strings.find(id);
    if (it != end(_strings))
        return it->second;
    throw std::runtime_error("no string present");
}

void TetrisConfig::load() {
    ptree pt;
    read_xml(configName, pt);
    orthographic = pt.get("tetris.<xmlattr>.orthographic", true);
    fullScreen = pt.get("tetris.<xmlattr>.fullscreen", false);
    monitor = pt.get("tetris.<xmlattr>.monitor", std::string());
    screenWidth = pt.get("tetris.resolution.<xmlattr>.width", 800);
    screenHeight = pt.get("tetris.resolution.<xmlattr>.height", 600);
    showFps = pt.get("tetris.<xmlattr>.showFps", false);
    initialLevel = pt.get("tetris.<xmlattr>.initialLevel", 0);
    language = pt.get("tetris.<xmlattr>.language", "en");
    readHighscores(highscoreLines, "tetris.lineHighscores", pt);
    readHighscores(highscoreScore, "tetris.scoreHighscores", pt);
    loadStrings();
}

void TetrisConfig::save() {
    ptree pt;
    pt.put("tetris.<xmlattr>.orthographic", orthographic);
    pt.put("tetris.<xmlattr>.fullscreen", fullScreen);
    pt.put("tetris.<xmlattr>.monitor", monitor);
    pt.put("tetris.resolution.<xmlattr>.width", screenWidth);
    pt.put("tetris.resolution.<xmlattr>.height", screenHeight);
    pt.put("tetris.<xmlattr>.showFps", showFps);
    pt.put("tetris.<xmlattr>.initialLevel", initialLevel);
    pt.put("tetris.<xmlattr>.language", language);
    writeHighscores(highscoreLines, "tetris.lineHighscores.highscore", pt);
    writeHighscores(highscoreScore, "tetris.scoreHighscores.highscore", pt);
    boost::property_tree::xml_writer_settings<std::string> settings('\t', 1);
    write_xml(configName, pt, std::locale(), settings);
}

#define X(s) { #s, StringID:: s },
std::map<std::string, StringID> stringNames = {
    STRING_ID_LIST
};
#undef X

void TetrisConfig::loadStrings() {
    ptree pt;
    std::string xmlName = "lang." + language + ".xml";
    read_xml(xmlName, pt);
    for (auto& node : pt.get_child("strings")) {
        std::string name = node.second.get("<xmlattr>.id", "");
        auto it = stringNames.find(name);
        if (it != end(stringNames)) {
            _strings[it->second] = node.second.get("<xmlattr>.value", "#NOVALUE");
        }
    }
}

bool HighscoreRecord::operator==(const HighscoreRecord &other) {
    return name == other.name &&
           lines == other.lines &&
           score == other.score &&
           initialLevel == other.initialLevel;
}
