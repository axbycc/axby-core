#include "wrappers/assimp.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>

void process_assimp_node(const aiScene* scene,
                         const aiNode* node,
                         axby::Mesh& data) {
    LOG(INFO) << "Processing node with " << node->mNumMeshes << " meshes";
    for (int mesh_idx = 0; mesh_idx < node->mNumMeshes; ++mesh_idx) {
        LOG(INFO) << "Node mesh " << mesh_idx << " located at global idx "
                  << node->mMeshes[mesh_idx];
        const aiMesh* mesh = scene->mMeshes[node->mMeshes[mesh_idx]];
        const int num_vertices = mesh->mNumVertices;
        const int num_color_channels = mesh->GetNumColorChannels();

        CHECK(num_color_channels == 1 || num_color_channels == 0);

        data.xyzs.reserve(num_vertices * 3);
        if (num_color_channels) {
            CHECK(mesh->HasVertexColors(0));
            data.rgbs.reserve(num_vertices * 3);
        }
        for (int i = 0; i < num_vertices; ++i) {
            const auto& vertex = mesh->mVertices[i];

            data.xyzs.push_back(vertex.x);
            data.xyzs.push_back(vertex.y);
            data.xyzs.push_back(vertex.z);

            if (num_color_channels) {
                auto& color = mesh->mColors[0][i];
                data.rgbs.push_back(uint8_t(255.5 * color.r));
                data.rgbs.push_back(uint8_t(255.5 * color.g));
                data.rgbs.push_back(uint8_t(255.5 * color.b));
                // ignore alpha for now
            }
        }
        if (mesh->HasFaces()) {
            data.faces.reserve(mesh->mNumFaces * 3);
            for (int i = 0; i < mesh->mNumFaces; ++i) {
                aiFace face = mesh->mFaces[i];
                CHECK(face.mNumIndices == 3);  // assume triangle faces
                for (int j = 0; j < face.mNumIndices; ++j) {
                    CHECK(face.mIndices[j] < num_vertices);
                    data.faces.push_back(face.mIndices[j]);
                }
            }
        }
    }

    for (int child_idx = 0; child_idx < node->mNumChildren; ++child_idx) {
        LOG(INFO) << "Processing child " << child_idx;
        process_assimp_node(scene, node->mChildren[child_idx], data);
    }
}

axby::MeshMaterial extract_assimp_material(const aiMaterial* material) {
    CHECK_NOTNULL(material);

    axby::MeshMaterial result;

    aiColor3D aicolor = {0, 0, 0};
    float aivalue = 0;

    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_DIFFUSE, aicolor)) {
        result.diffuse = {aicolor.r, aicolor.g, aicolor.b};
        // LOG(INFO) << "Material had diffuse " << result.diffuse;
    }

    // Specular color (Ks)
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_SPECULAR, aicolor)) {
        result.specular = {aicolor.r, aicolor.g, aicolor.b};
        // LOG(INFO) << "Material had specular" << result.specular;
    }

    // Ambient color (Ka)
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_AMBIENT, aicolor)) {
        result.ambient = {aicolor.r, aicolor.g, aicolor.b};
        // LOG(INFO) << "Material had ambient " << result.ambient;
    }

    // Emissive color (Ke)
    if (AI_SUCCESS == material->Get(AI_MATKEY_COLOR_EMISSIVE, aicolor)) {
        result.emissive = {aicolor.r, aicolor.g, aicolor.b};
        // LOG(INFO) << "Material had emissive " << result.emissive;
    }

    // Opacity (d)
    if (AI_SUCCESS == material->Get(AI_MATKEY_OPACITY, aivalue)) {
        result.opacity = aivalue;
        // LOG(INFO) << "Material had opacity " << result.opacity;
    }

    // Shininess (Ns)
    if (AI_SUCCESS == material->Get(AI_MATKEY_SHININESS, aivalue)) {
        result.specular_exponent = aivalue;
    } else if (AI_SUCCESS ==
               material->Get(AI_MATKEY_SHININESS_STRENGTH, aivalue)) {
        result.specular_exponent = aivalue;
    }

    return result;
}

std::vector<axby::Mesh> load_assimp_model_from_file(
    const std::string& filename) {
    Assimp::Importer importer;
    const aiScene* scene =
        importer.ReadFile(filename.data(), aiProcess_GenNormals);
    CHECK_NOTNULL(scene);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE ||
        !scene->mRootNode) {
        LOG(FATAL) << importer.GetErrorString();
    }

    CHECK(scene->mNumMeshes > 0)
        << "file " << filename << " contains no meshes";
    CHECK(scene->mNumMaterials > 0)
        << "file " << filename << " contains no materials";

    std::vector<axby::Mesh> mesh_parts{scene->mNumMeshes};

    for (int mesh_idx = 0; mesh_idx < scene->mNumMeshes; ++mesh_idx) {
        axby::Mesh& data = mesh_parts[mesh_idx];

        const aiMesh* mesh = scene->mMeshes[mesh_idx];
        const int num_vertices = mesh->mNumVertices;
        const int num_color_channels = mesh->GetNumColorChannels();
        const auto material_idx = mesh->mMaterialIndex;

        CHECK(material_idx < scene->mNumMaterials);
        const aiMaterial* aimaterial = scene->mMaterials[material_idx];
        data.material = extract_assimp_material(aimaterial);

        CHECK(num_color_channels == 1 || num_color_channels == 0);

        data.xyzs.reserve(num_vertices * 3);
        if (num_color_channels) {
            CHECK(mesh->HasVertexColors(0));
            data.rgbs.reserve(num_vertices * 3);
        }
        for (int i = 0; i < num_vertices; ++i) {
            const auto& vertex = mesh->mVertices[i];

            data.xyzs.push_back(vertex.x);
            data.xyzs.push_back(vertex.y);
            data.xyzs.push_back(vertex.z);

            if (num_color_channels) {
                auto& color = mesh->mColors[0][i];
                data.rgbs.push_back(uint8_t(255.5 * color.r));
                data.rgbs.push_back(uint8_t(255.5 * color.g));
                data.rgbs.push_back(uint8_t(255.5 * color.b));
                // ignore alpha for now
            }
        }
        if (mesh->HasFaces()) {
            data.faces.reserve(mesh->mNumFaces * 3);
            for (int i = 0; i < mesh->mNumFaces; ++i) {
                aiFace face = mesh->mFaces[i];
                CHECK(face.mNumIndices == 3);  // assume triangle faces
                for (int j = 0; j < face.mNumIndices; ++j) {
                    CHECK(face.mIndices[j] < num_vertices);
                    data.faces.push_back(face.mIndices[j]);
                }
            }
        }
        if (mesh->HasNormals()) {
            for (int i = 0; i < num_vertices; ++i) {
                const auto& normal = mesh->mNormals[i];
                data.normals.push_back(normal.x);
                data.normals.push_back(normal.y);
                data.normals.push_back(normal.z);
            }
        }
    }

    return mesh_parts;
}
