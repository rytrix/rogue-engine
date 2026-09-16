#include "model.hpp"

#include "mesh_compiler.hpp"

#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

#include "../utils/file.hpp"
#include "../utils/helpers.hpp"

#include "../app_data.hpp"

#include "../scene/resource_manager.hpp"

namespace Renderer {

class ModelLoader : public NoCopyNoMove {
public:
    ModelLoader() = default;
    ModelLoader(Mesh& mesh, const char* path, MeshLoaderFlags flags);
    ~ModelLoader();

    void init(const char* path);

    ModelResult m_error;

private:
    std::string m_directory;

    Mesh& m_mesh;

    MeshLoaderFlags m_flags;

    std::unordered_map<Utils::String, u32> m_texture_indices;

    void setup_mesh(const aiScene* scene);
    void setup_animations(const aiScene* scene);

    void process_node(aiNode* node, const aiScene* scene);
    void process_mesh(aiMesh* mesh, const aiScene* scene);

    u32 load_material_texture_memory(const aiMaterial* mat, const aiTextureType type, const aiScene* scene);
    Handle load_material_texture(const aiMaterial* mat, const aiTextureType type, const aiScene* scene);

    static std::string sanitize_path_to_filename(std::string path_str);
};

ModelResult load_mesh(Mesh& mesh, const char* path, MeshLoaderFlags flags)
{
    ModelLoader loader(mesh, path, flags);
    return loader.m_error;
}

ModelLoader::ModelLoader(Mesh& mesh, const char* file_path, MeshLoaderFlags flags)
    : m_mesh(mesh)
    , m_flags(flags)
{
    init(file_path);
    m_mesh.m_result = m_error;
}

void ModelLoader::init(const char* file_path)
{
    m_directory = file_path;

    if (!std::filesystem::exists(file_path)) {
        m_error.type = ModelResultEnum::InvalidFilePath;
        m_error.error.format("Model \"{}\" is an invalid path", file_path);
        return;
    }

    auto compiled_path = Utils::format(R"(res/cmodels/'{}'.rbin)", sanitize_path_to_filename(file_path));
    if (Utils::is_file_newer(compiled_path.c_str(), file_path)) {
        // load the compiled mesh instead
        bool result = Utils::read_file(m_mesh.m_serialized_bytes, compiled_path.c_str());
        if (result) {
            Renderer::load_compiled_mesh(m_mesh, { (u8*)m_mesh.m_serialized_bytes.data(), m_mesh.m_serialized_bytes.size() });
            return;
        }
    }

    m_mesh.m_path = file_path;

    m_directory = m_directory.substr(0, m_directory.find_last_of('/'));

    Assimp::Importer importer;
    importer.SetPropertyBool(AI_CONFIG_IMPORT_FBX_PRESERVE_PIVOTS, false);

    const aiScene* scene = importer.ReadFile(file_path,
        aiProcess_Triangulate
            | aiProcess_FlipUVs
            | aiProcess_CalcTangentSpace
            | aiProcess_JoinIdenticalVertices
            | aiProcess_GenBoundingBoxes);

    if (scene == nullptr) {
        const char* error = importer.GetErrorString();
        m_error.type = ModelResultEnum::UnknownError;
        m_error.error = error;
        return;
    }

    LOG_INFO(std::format("Loading Model from path {}", m_mesh.m_path.c_str()));

    setup_mesh(scene);
    setup_animations(scene);

    if (!Utils::is_file_newer(compiled_path.c_str(), file_path)) {
        // compile the mesh (it's already loaed)
        Utils::ByteStream bytes;
        Renderer::compile_mesh(bytes, m_mesh);
        bool result = Utils::write_file(compiled_path.c_str(), { (char*)bytes.data(), bytes.size() });
        util_assert(result, std::format("failed to write to \"{}\"", compiled_path.c_str()));
        m_mesh.drop_texture_memory();
    }

}

void ModelLoader::setup_mesh(const aiScene* scene)
{
    process_node(scene->mRootNode, scene);

    usize offset = 0;
    for (usize i = 0; i < m_mesh.m_vertex_data.m_base_vertices.size(); i++) {
        m_mesh.m_vertex_data.m_base_vertices.at(i).m_offset = offset;
        offset += m_mesh.m_vertex_data.m_base_vertices.at(i).m_count;
    }

    m_mesh.m_vertex_data_view.m_base_vertices = m_mesh.m_vertex_data.m_base_vertices;
    m_mesh.m_vertex_data_view.m_vertices = m_mesh.m_vertex_data.m_vertices;
    m_mesh.m_vertex_data_view.m_indices = m_mesh.m_vertex_data.m_indices;
    m_mesh.m_vertex_data_view.m_bones = m_mesh.m_vertex_data.m_bones;


    if (has_flag(m_flags, MeshLoaderFlags::StoreTextures) && has_flag(m_flags, MeshLoaderFlags::UploadTexturesToGPU)) {
        m_mesh.upload_texture_memory_to_gpu();
    }

    if (!has_flag(m_flags, MeshLoaderFlags::DontInitializeOpenGL)) {
        m_mesh.setup_mesh();
    }
}

void ModelLoader::setup_animations(const aiScene* scene)
{
    glm::mat4 global_inverse_transform = glm::inverse(mat4_to_mat4(scene->mRootNode->mTransformation));

    m_mesh.m_animations.resize(scene->mNumAnimations);

    for (u32 i = 0; i < scene->mNumAnimations; i++) {
        auto* animation = scene->mAnimations[i];
        LOG_INFO(std::format("Animation info: Name: {}, Duration: {}, Ticks Per Second: {}", animation->mName.C_Str(), animation->mDuration, animation->mTicksPerSecond));

        m_mesh.m_animations[i].init(scene,
            animation,
            m_mesh.m_bone_id_map,
            global_inverse_transform);
    }
}

ModelLoader::~ModelLoader()
{
}

void ModelLoader::process_node(aiNode* node, const aiScene* scene)
{
    for (u32 i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        LOG_INFO(std::format("Loading mesh: {}", node->mMeshes[i]));
        process_mesh(mesh, scene);
    }

    for (u32 i = 0; i < node->mNumChildren; i++) {
        process_node(node->mChildren[i], scene);
    }
}

void ModelLoader::process_mesh(aiMesh* mesh, const aiScene* scene)
{
    auto base_vertex = static_cast<GLsizei>(m_mesh.m_vertex_data.m_vertices.size());
    auto count = static_cast<GLsizei>(m_mesh.m_vertex_data.m_indices.size());

    for (u32 i = 0; i < mesh->mNumVertices; i++) {
        Mesh::Vertex& vertex = m_mesh.m_vertex_data.m_vertices.emplace_back();

        vertex.m_pos.x = mesh->mVertices[i].x;
        vertex.m_pos.y = mesh->mVertices[i].y;
        vertex.m_pos.z = mesh->mVertices[i].z;

        vertex.m_norm.x = mesh->mNormals[i].x;
        vertex.m_norm.y = mesh->mNormals[i].y;
        vertex.m_norm.z = mesh->mNormals[i].z;

        vertex.m_tang.x = mesh->mTangents[i].x;
        vertex.m_tang.y = mesh->mTangents[i].y;
        vertex.m_tang.z = mesh->mTangents[i].z;

        if (mesh->HasTextureCoords(0)) {
            vertex.m_tex.x = mesh->mTextureCoords[0][i].x;
            vertex.m_tex.y = mesh->mTextureCoords[0][i].y;
        }
    }

    for (u32 i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];

        for (u32 j = 0; j < face.mNumIndices; j++) {
            m_mesh.m_vertex_data.m_indices.push_back(face.mIndices[j]);
        }
    }

    if (scene->HasMaterials()) {
        aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];

        if (has_flag(m_flags, MeshLoaderFlags::StoreTextures)) {
            auto diffuse_map = load_material_texture_memory(material, aiTextureType_DIFFUSE, scene);
            m_mesh.m_texture_data.m_diffuse_textures_memory.push_back(diffuse_map);

            auto metallic_map = load_material_texture_memory(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, scene);
            m_mesh.m_texture_data.m_metallic_roughness_textures_memory.push_back(metallic_map);

            auto normal_map = load_material_texture_memory(material, aiTextureType_NORMALS, scene);
            m_mesh.m_texture_data.m_normal_textures_memory.push_back(normal_map);
        } else if (has_flag(m_flags, MeshLoaderFlags::UploadTexturesToGPU)) {
            auto diffuse_map = load_material_texture(material, aiTextureType_DIFFUSE, scene);
            if (diffuse_map.generation.valid == 0) {
                LOG_WARN("Using default albedo texture map");
                m_mesh.m_texture_data.m_diffuse_textures.push_back(g_global_app_data->m_default_textures->get_albedo());
            } else {
                m_mesh.m_texture_data.m_diffuse_textures.push_back(diffuse_map);
            }

            auto metallic_roughness_map = load_material_texture(material, aiTextureType_GLTF_METALLIC_ROUGHNESS, scene);
            if (metallic_roughness_map.generation.valid == 0) {
                LOG_WARN("Using default metallic texture map");
                m_mesh.m_texture_data.m_metallic_roughness_textures.push_back(g_global_app_data->m_default_textures->get_metallic());
            } else {
                m_mesh.m_texture_data.m_metallic_roughness_textures.push_back(metallic_roughness_map);
            }

            auto normal_map = load_material_texture(material, aiTextureType_NORMALS, scene);
            if (normal_map.generation.valid == 0) {
                LOG_WARN("Using default normal texture map");
                m_mesh.m_texture_data.m_normal_textures.push_back(g_global_app_data->m_default_textures->get_normal());
            } else {
                m_mesh.m_texture_data.m_normal_textures.push_back(normal_map);
            }
        }
    }

    if (mesh->HasBones()) {
        m_mesh.m_has_bones = true;
        m_mesh.m_vertex_data.m_bones.resize(m_mesh.m_vertex_data.m_bones.size() + mesh->mNumVertices);

        for (u32 i = 0; i < mesh->mNumBones; i++) {
            // Each bone has mName, mNumWeights, mOffsetMatrix, mWeights

            // Reverse map each vertex to the bones that influence it (to be accessed in the vertex shader)
            // preferrably as a seperate data structure

            // Each bone has a ID (and its mat4 transformation)
            // Each vertex has a list of IDs and Weights that correspond to those bones

            aiBone* bone = mesh->mBones[i];

            usize bone_id;

            // There might be multiple meshes with the same bones
            if (!m_mesh.m_bone_id_map.contains(bone->mName.C_Str())) {
                bone_id = m_mesh.m_bone_id_map.size();
                m_mesh.m_bone_id_map[bone->mName.C_Str()] = bone_id;

                LOG_DEBUG(std::format("Loaded bone: \"{}\" at index {}", bone->mName.C_Str(), i));
            } else {
                bone_id = m_mesh.m_bone_id_map[bone->mName.C_Str()];
            }

            for (u32 j = 0; j < bone->mNumWeights; j++) {
                const aiVertexWeight vw = bone->mWeights[j];
                m_mesh.m_vertex_data.m_bones
                    .at(base_vertex + vw.mVertexId)
                    .add_bone(bone_id, vw.mWeight);
            }
        }
    }

    auto& aabb = m_mesh.m_aabbs.emplace_back();
    aabb.min.x = mesh->mAABB.mMin.x;
    aabb.min.y = mesh->mAABB.mMin.y;
    aabb.min.z = mesh->mAABB.mMin.z;

    aabb.max.x = mesh->mAABB.mMax.x;
    aabb.max.y = mesh->mAABB.mMax.y;
    aabb.max.z = mesh->mAABB.mMax.z;

    count = static_cast<GLsizei>(m_mesh.m_vertex_data.m_indices.size()) - count;

    m_mesh.m_vertex_data.m_base_vertices.emplace_back(
        count,
        base_vertex);
}

u32 ModelLoader::load_material_texture_memory(const aiMaterial* mat, const aiTextureType type, const aiScene* scene)
{
    if (mat->GetTextureCount(type) > 0) {
        aiString str;
        mat->GetTexture(type, 0, &str);
        const aiTexture* embedded_texture = scene->GetEmbeddedTexture(str.C_Str());

        TextureMemoryInfo memory_info;
        memory_info.flip = false;
        if (embedded_texture == nullptr) {
            Utils::String texture_path = Utils::format("{}/{}", m_directory.c_str(), str.C_Str());
            LOG_INFO(std::format("Loading {} type {}", texture_path.view(), aiTextureTypeToString(type)));

            if (m_texture_indices.contains(texture_path)) {
                return m_texture_indices[texture_path];
            } else {
                memory_info.origin = TextureOrigin::File;
                memory_info.file_path = texture_path.c_str();

                auto& memory = m_mesh.m_texture_data.m_texture_memory.emplace_back();
                memory.init(memory_info);
                u32 index = m_mesh.m_texture_data.m_texture_memory.size() - 1;
                m_texture_indices[texture_path] = index;
                return index;
            }
        } else {
            Utils::String texture_path(str.C_Str());
            LOG_INFO(std::format("Loading {} type {}", texture_path.view(), aiTextureTypeToString(type)));

            if (m_texture_indices.contains(texture_path)) {
                return m_texture_indices[texture_path];
            } else {
                memory_info.origin = TextureOrigin::Memory;
                memory_info.memory = (char*)embedded_texture->pcData;
                memory_info.memory_size = embedded_texture->mWidth;

                auto& memory = m_mesh.m_texture_data.m_texture_memory.emplace_back();
                memory.init(memory_info);
                u32 index = m_mesh.m_texture_data.m_texture_memory.size() - 1;
                m_texture_indices[texture_path] = index;
                return index;
            }
        }
    }

    return UINT32_MAX;
}

Handle ModelLoader::load_material_texture(const aiMaterial* mat, const aiTextureType type, const aiScene* scene)
{
    if (mat->GetTextureCount(type) > 0) {
        aiString str;
        mat->GetTexture(type, 0, &str);

        const aiTexture* embedded_texture = scene->GetEmbeddedTexture(str.C_Str());

        TextureInfo texture_info {};
        texture_info.min_filter = GL_LINEAR_MIPMAP_LINEAR;
        texture_info.mag_filter = GL_LINEAR;
        texture_info.mipmaps = true;
        texture_info.mipmap_levels = 0;
        texture_info.memory_info.flip = false;

        auto* texture_cache = g_global_app_data->m_texture_cache;
        if (embedded_texture == nullptr) {
            Utils::String texture_path = Utils::format("{}/{}", m_directory.c_str(), str.C_Str());

            texture_info.memory_info.origin = TextureOrigin::File;
            texture_info.memory_info.file_path = texture_path.c_str();

            LOG_INFO(std::format("Loading {} type {}", texture_path.view(), aiTextureTypeToString(type)));

            Handle handle;
            if (texture_cache->contains(texture_path)) {
                handle = texture_cache->get(texture_path);
            } else {
                handle = texture_cache->get_or_create(texture_path, texture_info);
                auto* texture = texture_cache->get(handle);
                texture->set_max_anisotropy(16.0F);
            }
            return handle;
        } else {
            Utils::String texture_path(str.C_Str());

            LOG_INFO(std::format("Loading {} type {}", texture_path.view(), aiTextureTypeToString(type)));
            if (texture_cache->contains(texture_path)) {
                return texture_cache->get(texture_path);
            } else {
                texture_info.memory_info.origin = TextureOrigin::Memory;
                texture_info.memory_info.memory = (char*)embedded_texture->pcData;
                texture_info.memory_info.memory_size = embedded_texture->mWidth;

                auto handle = texture_cache->get_or_create(texture_path, texture_info);
                Texture* texture = texture_cache->get(handle);
                texture->set_max_anisotropy(16.0F);

                return handle;
            }
        }
    } else {
        return { .generation = { .valid = 0, .id = 0 }, .index = 0 };
    }
}

std::string ModelLoader::sanitize_path_to_filename(std::string path_str)
{
    std::ranges::replace(path_str, '/', '-');
    std::ranges::replace(path_str, '\\', '-');
    return path_str;
}

} // namespace Renderer
