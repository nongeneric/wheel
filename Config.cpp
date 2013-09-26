#include "Config.h"

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

/*
<tetris orthographic="true"
        fullscreen="true"
        showFps="false"
        initialLevel="17">
    <resolution width="1920"
                height="1080"/>
</tetris>
*/

void TetrisConfig::load(const std::string &fileName) {
    using boost::property_tree::ptree;
    ptree pt;
    read_xml(fileName, pt);
    orthographic = pt.get("tetris.<xmlattr>.orthographic", true);
    fullScreen = pt.get("tetris.<xmlattr>.fullscreen", false);
    screenWidth = pt.get("tetris.resolution.<xmlattr>.width", 600);
    screenHeight = pt.get("tetris.resolution.<xmlattr>.height", 600);
    showFps = pt.get("tetris.<xmlattr>.showFps", false);
    initialLevel = pt.get("tetris.<xmlattr>.initialLevel", 0);
}
