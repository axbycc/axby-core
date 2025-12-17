#pragma once

#include <assimp/scene.h>

#include <vector>

#include "viewer/mesh.h"

axby::Mesh process_assimp_node(const aiScene* scene, const aiNode* node);
axby::MeshMaterial extract_assimp_material(const aiMaterial* material);
std::vector<axby::Mesh> load_assimp_model_from_file(const std::string& filename);
