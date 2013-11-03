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

void readHighscores(std::vector<HighscoreRecord>& vec, std::string path, ptree& pt) {
    for (auto& node : pt.get_child(path)) {
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

void TetrisConfig::load(const std::string &fileName) {
    ptree pt;
    read_xml(fileName, pt);
    orthographic = pt.get("tetris.<xmlattr>.orthographic", true);
    fullScreen = pt.get("tetris.<xmlattr>.fullscreen", false);
    screenWidth = pt.get("tetris.resolution.<xmlattr>.width", 800);
    screenHeight = pt.get("tetris.resolution.<xmlattr>.height", 600);
    showFps = pt.get("tetris.<xmlattr>.showFps", false);
    initialLevel = pt.get("tetris.<xmlattr>.initialLevel", 0);
    readHighscores(highscoreLines, "tetris.lineHighscores", pt);
    readHighscores(highscoreScore, "tetris.scoreHighscores", pt);
}

void TetrisConfig::save(const std::string &fileName) {
    ptree pt;
    pt.put("tetris.<xmlattr>.orthographic", orthographic);
    pt.put("tetris.<xmlattr>.fullscreen", fullScreen);
    pt.put("tetris.resolution.<xmlattr>.width", screenWidth);
    pt.put("tetris.resolution.<xmlattr>.height", screenHeight);
    pt.put("tetris.<xmlattr>.showFps", showFps);
    pt.put("tetris.<xmlattr>.initialLevel", initialLevel);
    writeHighscores(highscoreLines, "tetris.lineHighscores.highscore", pt);
    writeHighscores(highscoreScore, "tetris.scoreHighscores.highscore", pt);
    boost::property_tree::xml_writer_settings<char> settings('\t', 1);
    write_xml(fileName, pt, std::locale(), settings);
}

bool HighscoreRecord::operator==(const HighscoreRecord &other) {
    return name == other.name &&
           lines == other.lines &&
           score == other.score &&
           initialLevel == other.initialLevel;
}
