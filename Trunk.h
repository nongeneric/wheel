#pragma once

#include "Tetris.h"
#include "Texture.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <boost/chrono.hpp>
#include <vector>
#include <functional>

using fseconds = boost::chrono::duration<float>;

struct TrunkCube {
    Mesh mesh;
    CellInfo info;
    TrunkCube(Mesh& mesh, CellInfo info = CellInfo{}) : mesh(mesh), info(info) { }
};

class Program;
class Trunk {
    std::vector<TrunkCube> _cubes;
    std::vector<TrunkCube> _border;
    glm::mat4 _pos;
    int _hor, _vert;
    bool _drawBorder;
    Texture _texBorder;
    std::vector<Texture> _pieceColors;
    TrunkCube& at(int x, int y);
public:
    Trunk(int hor, int vert, bool drawBorder);
    void animateDestroy(int x, int y, fseconds duration);
    void hide(int x, int y);
    void show(int x, int y);
    void setCellInfo(int x, int y, CellInfo& info);
    friend void animate(Trunk& trunk, fseconds dt);
    friend void draw(Trunk&, int, int, glm::mat4, Program&);
    friend void setPos(Trunk&, glm::vec3);
    friend glm::mat4 const& getPos(Trunk&);
};

void draw(Trunk& t, int mv_location, int mvp_location, glm::mat4 vp, Program& program);
void setPos(Trunk& trunk, glm::vec3 pos);
void animate(Trunk& trunk, fseconds dt);
fseconds copyState(
        Tetris& tetris,
        std::function<CellInfo(Tetris&, int x, int y)> getter,
        int xMax,
        int yMax,
        Trunk& trunk,
        fseconds duration);
