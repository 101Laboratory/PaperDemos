#include "Model.h"

#include <assert.h>
#include <assimp/postprocess.h>  // Post processing flags
#include <assimp/scene.h>        // Output data structure

#include <assimp/Importer.hpp>  // C++ importer interface
#include <stdexcept>

ObjModel::ObjModel(std::string file) {
  Assimp::Importer importer;

  const aiScene* scene = importer.ReadFile(
      file.c_str(),
      aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipWindingOrder |
          aiProcess_MakeLeftHanded | aiProcess_GenSmoothNormals);

  if (!scene)
    throw std::runtime_error{"failed to load obj model from file: " + file};

  if (scene->HasMeshes()) {
    for (int i = 0; i < scene->mNumMeshes; ++i) {
      auto& mesh = scene->mMeshes[i];

      assert(mesh->HasPositions(), "mesh does not have positions");
      assert(mesh->HasNormals(), "mesh does not have normals");
      assert(mesh->HasFaces(), "mesh does not have faces");

      // Populate vertices
      for (int j = 0; j < mesh->mNumVertices; ++j) {
        Vertex v;

        const auto& v0 = mesh->mVertices[j];
        v.pos = {v0.x, v0.y, v0.z};

        const auto& n0 = mesh->mNormals[j];
        v.normal = {n0.x, n0.y, n0.z};

        vertices_.push_back(v);
      }

      // Populate indices
      for (int j = 0; j < mesh->mNumFaces; ++j) {
        const auto& f = mesh->mFaces[j];
        for (int k = 0; k < f.mNumIndices; ++k) {
          indices_.push_back(static_cast<std::uint16_t>(f.mIndices[k]));
        }
      }
    }
  }
}