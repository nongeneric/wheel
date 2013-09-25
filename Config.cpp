#include "Config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

/*
<tetris>
    <orthographic>true</orthographic>
    <fullscreen>true</fullscreen>
    <showFps>false</showFps>
    <initialLevel>17</initialLevel>
    <resolution>
        <width>1920</width>
        <height>1080</height>
    </resolution>
</tetris>
*/

void TetrisConfig::load(const std::string &fileName) {
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(fileName, pt);
    orthographic = pt.get("tetris.orthographic", true);
    fullScreen = pt.get("tetris.fullscreen", false);
    screenWidth = pt.get("tetris.resolution.width", 600);
    screenHeight = pt.get("tetris.resolution.height", 600);
    showFps = pt.get("tetris.showFps", false);
    initialLevel = pt.get("tetris.initialLevel", 0);
}
