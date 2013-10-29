#include "Mesh.h"

#include <glm/gtc/matrix_transform.hpp>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

void init_vao(VertexBuffer buffer) {
    buffer.bind();
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)12);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)20);
}


Mesh::Mesh(VertexBuffer vertices, IndexBuffer indices) {
    _indicesCount = indices.size();
    BindLock<VAO> vaoLock(_vao);
    init_vao(vertices);
    indices.bind();
}


glm::mat4 getTransform(Mesh &mesh) {
    return mesh._pos * mesh._scale;
}


void draw(Mesh &mesh, int world_location, int mvp_location, glm::mat4 vp, Program &program) {
    BindLock<VAO> vaoLock(mesh._vao);
    glm::mat4 mvp = vp * getTransform(mesh);
    program.setUniform(mvp_location, mvp);
    program.setUniform(world_location, getTransform(mesh));
    glDrawElements(GL_TRIANGLES, mesh._indicesCount, GL_UNSIGNED_SHORT, 0);
}


void setPos(Mesh &mesh, glm::vec3 pos) {
    mesh._pos = glm::translate( {}, pos );
}


void setScale(Mesh &mesh, glm::vec3 scale) {
    mesh._scale = glm::scale({}, scale);
}


void setAnimation(Mesh &mesh, ScaleAnimation a) {
    mesh._animation = a;
}


void animate(Mesh &mesh, fseconds dt) {
    mesh._animation.animate(dt, [&]() -> Mesh& { return mesh; });
}


Mesh loadMesh() {
    Assimp::Importer importer;
    const aiScene* scene = importer.ReadFile("cube.ply",
                                             aiProcess_CalcTangentSpace       |
                                             aiProcess_Triangulate            |
                                             aiProcess_JoinIdenticalVertices);
    if (!scene) {
        std::cout << importer.GetErrorString();
        throw std::runtime_error("can't load a mesh");
    }
    assert(scene->HasMeshes());
    assert(scene->mNumMeshes == 1);
    std::vector<Vertex> vertices;
    aiMesh* mesh = scene->mMeshes[0];
    assert(mesh->HasNormals());
    for (size_t i = 0; i < mesh->mNumVertices; ++i) {
        vertices.push_back({
                               glm::vec3(mesh->mVertices[i].x,
                               mesh->mVertices[i].y,
                               mesh->mVertices[i].z),
                               glm::vec2(),
                               glm::vec3(mesh->mNormals[i].x,
                               mesh->mNormals[i].y,
                               mesh->mNormals[i].z)});
    }
    std::vector<short> indices;
    for (size_t i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        assert(face.mNumIndices == 3);
        indices.push_back(face.mIndices[0]);
        indices.push_back(face.mIndices[1]);
        indices.push_back(face.mIndices[2]);
    }
    return Mesh(VertexBuffer(vertices), IndexBuffer(indices));
}
