#include "Trunk.h"

#include <glm/gtc/matrix_transform.hpp>

const float cubeSpace = 0.2f;

Texture oneColorTex(uint32_t color) {
    Texture tex;
    char *c = reinterpret_cast<char*>(&color);
    std::swap(c[0], c[3]);
    std::swap(c[1], c[2]);
    BitmapPtr bitmap(FreeImage_Allocate(1, 1, 32));
    FreeImage_SetPixelColor(bitmap.get(), 0, 0, (RGBQUAD*)&color);
    tex.setImage(bitmap);
    return tex;
}

TrunkCube &Trunk::at(int x, int y) {
    return _cubes.at(_hor * y + x);
}

Trunk::Trunk(int hor, int vert, bool drawBorder)
    : _hor(hor),
      _vert(vert),
      _drawBorder(drawBorder),
      _texBorder(oneColorTex(0xFFFFFFFF))
{
    _pieceColors.resize(PieceType::count);
    _pieceColors[PieceType::I] = oneColorTex(0xFF0000FF);
    _pieceColors[PieceType::J] = oneColorTex(0xFFFFFFFF);
    _pieceColors[PieceType::L] = oneColorTex(0xffa500FF);
    _pieceColors[PieceType::O] = oneColorTex(0xffff00FF);
    _pieceColors[PieceType::S] = oneColorTex(0xff00ffFF);
    _pieceColors[PieceType::T] = oneColorTex(0x00ffffFF);
    _pieceColors[PieceType::Z] = oneColorTex(0x00FF00FF);

    float xOffset = (1.0 * (hor + 2) + cubeSpace * (hor + 1) - 0.5) / -2.0f;
    for (int y = 0; y < vert; ++y) {
        for (int x = 0; x < hor; ++x) {
            Mesh cube = loadMesh();
            float xPos = xOffset + (cubeSpace + 1) * (x + 1);
            float yPos = y * (1 + cubeSpace) + cubeSpace;
            setPos(cube, glm::vec3 { xPos, yPos, 0 });
            _cubes.emplace_back(cube);
        }
    }
    if (!_drawBorder)
        return;
    for (int y = 0; y < vert; ++y) {
        Mesh left = loadMesh();
        setPos(left, glm::vec3 { xOffset, y * (1 + cubeSpace) + cubeSpace, 0 });
        _border.emplace_back(left);
        Mesh right = loadMesh();
        float xPos = xOffset + (hor + 1) * (1 + cubeSpace);
        float yPos = y * (1 + cubeSpace) + cubeSpace;
        setPos(right, glm::vec3 { xPos, yPos, 0 });
        _border.emplace_back(right);
    }
}

void Trunk::animateDestroy(int x, int y, fseconds duration) {
    setAnimation(at(x, y).mesh, ScaleAnimation(duration, 1, 0));
}

void Trunk::hide(int x, int y) {
    setScale(at(x, y).mesh, glm::vec3 {0, 0, 0});
}

void Trunk::show(int x, int y) {
    setScale(at(x, y).mesh, glm::vec3 {1, 1, 1});
}

void Trunk::setCellInfo(int x, int y, CellInfo &info) {
    at(x, y).info = info;
}


void draw(Trunk &t, int mv_location, int mvp_location, glm::mat4 vp, Program &program) {
    for (TrunkCube& cube : t._cubes) {
        assert((unsigned)cube.info.piece < PieceType::count);
        BindLock<Texture> texLock(t._pieceColors.at(cube.info.piece));
        ::draw(cube.mesh, mv_location, mvp_location, vp * t._pos, program);
    }
    BindLock<Texture> texLock(t._texBorder);
    for (TrunkCube& cube : t._border)
        ::draw(cube.mesh, mv_location, mvp_location, vp * t._pos, program);
}


void setPos(Trunk &trunk, glm::vec3 pos) {
    trunk._pos = glm::translate({}, pos);
}


void animate(Trunk &trunk, fseconds dt) {
    for (TrunkCube& cube : trunk._cubes)
        animate(cube.mesh, dt);
}


fseconds copyState(Tetris &tetris, std::function<CellInfo (Tetris &, int, int)> getter, int xMax, int yMax, Trunk &trunk, fseconds duration)
{
    bool dying = false;
    for (int x = 0; x < xMax; ++x) {
        for (int y = 0; y < yMax; ++y) {
            CellInfo cellInfo = getter(tetris, x, y);
            assert((unsigned)cellInfo.piece < PieceType::count);
            trunk.setCellInfo(x, y, cellInfo);
            switch (cellInfo.state) {
            case CellState::Shown:
                trunk.show(x, y);
                break;
            case CellState::Hidden:
                trunk.hide(x, y);
                break;
            case CellState::Dying:
                dying = true;
                trunk.animateDestroy(x, y, duration);
                break;
            default: assert(false);
            }
        }
    }
    return dying ? duration : fseconds();
}
