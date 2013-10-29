#pragma once

#include "OpenGLbasics.h"
#include "ScaleAnimation.h"
#include "Program.h"
#include <glm/glm.hpp>

void init_vao(VertexBuffer buffer);

class Mesh {
    VAO _vao;
    unsigned _indicesCount;
    glm::mat4 _pos;
    glm::mat4 _scale;
    ScaleAnimation _animation;
public:
    Mesh(VertexBuffer vertices, IndexBuffer indices);
    // assumes a program is bound, doesn't manage its position
    friend void draw(Mesh& mesh, int mv_location, int mvp_location, glm::mat4 vp, Program& program);
    friend void setPos(Mesh& mesh, glm::vec3 pos);
    friend void setScale(Mesh& mesh, glm::vec3 scale);
    friend void setAnimation(Mesh& mesh, ScaleAnimation a);
    friend void animate(Mesh& mesh, fseconds dt);
    friend glm::mat4 getTransform(Mesh& mesh);
    glm::mat4 const& getPos() const {
        return _pos;
    }
};

glm::mat4 getTransform(Mesh& mesh);
void draw(Mesh& mesh, int world_location, int mvp_location, glm::mat4 vp, Program& program);
void setPos(Mesh& mesh, glm::vec3 pos);
void setScale(Mesh& mesh, glm::vec3 scale);
void setAnimation(Mesh& mesh, ScaleAnimation a);
void animate(Mesh& mesh, fseconds dt);
Mesh loadMesh();
