#include "mesh_compiler.hpp"

#include "../utils/bytestream.hpp"

#include "mesh.hpp"
#include "model.hpp"

namespace Renderer {

struct GlobalHeader {
    char magic_number[6] = "CMESH";
    u16 version = 1;
    u64 size = 0;
    u64 offset_to_base_vertices = 0;
    u64 offset_to_vertices = 0;
    u64 offset_to_indices = 0;
    u64 offset_to_textures = 0;
    u64 offset_to_bones = 0;
    u64 offset_to_animations = 0;
    u64 offset_to_aabb = 0;
    char mesh_name[Utils::String::capacity()] = "unnamed mesh";
};

struct BoneNameID {
    Utils::String name;
    u32 id;

    void serialize(Utils::ByteStream& stream);
    [[nodiscard]] u8* deserialize(u8* current_ptr);
};

void BoneNameID::serialize(Utils::ByteStream& stream)
{
    u64 size = name.size();
    stream.append_bytes(&size, sizeof(size));
    stream.append_bytes(name.data(), name.capacity());
    stream.append_bytes(&id, sizeof(id));
}

[[nodiscard]] u8* BoneNameID::deserialize(u8* current_ptr)
{
    u64 size = 0;

    std::memcpy(&size, current_ptr, sizeof(u64));
    current_ptr += sizeof(size);

    name = (char*)current_ptr;
    current_ptr += name.capacity();

    std::memcpy(&id, current_ptr, sizeof(u32));
    current_ptr += sizeof(u32);

    return current_ptr;
}

class MeshCompiler : public NoCopyNoMove {
public:
    MeshCompiler() = default;
    MeshCompiler(Utils::ByteStream& bytes);

    void serialize(Mesh& mesh);
    void deserialize(Mesh& mesh, const std::span<u8> compiled_mesh);

private:
    Utils::ByteStream* m_bytes = nullptr;

    GlobalHeader* get_global_header();
};

MeshCompiler::MeshCompiler(Utils::ByteStream& bytes)
    : m_bytes(&bytes)
{
}

void compile_mesh(Utils::ByteStream& bytes, const char* file_path)
{
    Renderer::Mesh mesh;
    Renderer::load_mesh(mesh, file_path, MeshLoaderFlags::StoreTextures | MeshLoaderFlags::DontInitializeOpenGL);
    ModelResult result = mesh.get_result();
    if (result.type != ModelResultEnum::Ok) {
        LOG_ERROR(std::format("{}", result.error.c_str()));
        return;
    }

    MeshCompiler compiler(bytes);
    compiler.serialize(mesh);
}

void compile_mesh(Utils::ByteStream& bytes, Mesh& mesh)
{
    MeshCompiler compiler(bytes);
    compiler.serialize(mesh);
}

void load_compiled_mesh(Mesh& mesh, const std::span<u8> compiled_mesh)
{
    MeshCompiler compiler;
    compiler.deserialize(mesh, compiled_mesh);
}

void MeshCompiler::serialize(Mesh& mesh)
{
    util_assert(m_bytes != nullptr, "m_bytes is nullptr");

    GlobalHeader _temp_initial_header;
    m_bytes->append_bytes(&_temp_initial_header, sizeof(_temp_initial_header));
    LOG_TRACE(std::format("serializing \"{}\"", mesh.m_path.c_str()));

    // Base Vertices
    u64 base_vertices_offset = m_bytes->size();

    u64 size = mesh.m_vertex_data.m_base_vertices.size();
    m_bytes->append_bytes(&size, sizeof(size));
    m_bytes->append_bytes(mesh.m_vertex_data.m_base_vertices.data(), size * sizeof(mesh.m_vertex_data.m_base_vertices[0]));
    LOG_TRACE(std::format("appended {} base vertices", size));

    // Vertices
    u64 vertices_offset = m_bytes->size();

    size = mesh.m_vertex_data.m_vertices.size();
    m_bytes->append_bytes(&size, sizeof(size));
    m_bytes->append_bytes(mesh.m_vertex_data.m_vertices.data(), size * sizeof(mesh.m_vertex_data.m_vertices[0]));
    LOG_TRACE(std::format("appended {} vertices", size));

    // Indices
    u64 indices_offset = m_bytes->size();

    size = mesh.m_vertex_data.m_indices.size();
    m_bytes->append_bytes(&size, sizeof(size));
    m_bytes->append_bytes(mesh.m_vertex_data.m_indices.data(), size * sizeof(mesh.m_vertex_data.m_indices[0]));
    LOG_TRACE(std::format("appended {} indices", size));

    // Textures
    u64 textures_offset = m_bytes->size();

    size = mesh.m_texture_data.m_texture_memory.size();
    m_bytes->append_bytes(&size, sizeof(size));
    for (usize i = 0; i < size; i++) {
        mesh.m_texture_data.m_texture_memory[i].serialize(*m_bytes);
    }
    LOG_TRACE(std::format("appended {} textures", size));

    size = mesh.m_texture_data.m_diffuse_textures_memory.size();
    m_bytes->append_bytes(&size, sizeof(size));
    m_bytes->append_bytes(mesh.m_texture_data.m_diffuse_textures_memory.data(), size * sizeof(u32));
    m_bytes->append_bytes(mesh.m_texture_data.m_metallic_roughness_textures_memory.data(), size * sizeof(u32));
    m_bytes->append_bytes(mesh.m_texture_data.m_normal_textures_memory.data(), size * sizeof(u32));
    LOG_TRACE(std::format("appended {} texture indices", size));

    // Bones
    u64 bones_offset = m_bytes->size();

    size = mesh.m_vertex_data.m_bones.size();
    m_bytes->append_bytes(&size, sizeof(size));
    m_bytes->append_bytes(mesh.m_vertex_data.m_bones.data(), size * sizeof(mesh.m_vertex_data.m_bones[0]));
    LOG_TRACE(std::format("appended {} bones vertices", size));

    size = mesh.m_bone_id_map.size();
    m_bytes->append_bytes(&size, sizeof(size));
    for (auto& bone_id : mesh.m_bone_id_map) {
        BoneNameID bone { .name = bone_id.first, .id = bone_id.second };
        bone.serialize(*m_bytes);
    }
    LOG_TRACE(std::format("appended {} bones", size));

    // Animations
    m_bytes->align();
    u64 animations_offset = m_bytes->size();

    u64 total_animations = mesh.m_animations.size();
    m_bytes->append_bytes(&total_animations, sizeof(total_animations));
    for (auto& animation : mesh.m_animations) {
        animation.serialize(*m_bytes);
    }
    LOG_TRACE(std::format("appended {} animations", total_animations));

    // AABB
    u64 aabb_offset = m_bytes->size();
    m_bytes->append_bytes(&mesh.m_aabb, sizeof(mesh.m_aabb));

    auto* global_header = get_global_header();
    global_header->size = m_bytes->size();
    global_header->offset_to_base_vertices = base_vertices_offset;
    global_header->offset_to_vertices = vertices_offset;
    global_header->offset_to_indices = indices_offset;
    global_header->offset_to_textures = textures_offset;
    global_header->offset_to_bones = bones_offset;
    global_header->offset_to_animations = animations_offset;
    global_header->offset_to_aabb = aabb_offset;
    memcpy(global_header->mesh_name, mesh.m_path.data(), mesh.m_path.capacity());
}

void MeshCompiler::deserialize(Mesh& mesh, const std::span<u8> compiled_mesh)
{
    GlobalHeader* header = (GlobalHeader*)compiled_mesh.data();
    if (strcmp(header->magic_number, "CMESH") != 0) {
        mesh.m_result = ModelResult {
            .type = ModelResultEnum::InvalidFormat,
            .error = "Invalid file format"
        };
    }

    memcpy(mesh.m_path.data(), header->mesh_name, mesh.m_path.capacity());

    // Base Vertices
    util_assert(compiled_mesh.size() > header->offset_to_base_vertices, "CMESH invalid size");
    u8* ptr = compiled_mesh.data() + header->offset_to_base_vertices;

    u64 size = 0;
    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_vertex_data_view.m_base_vertices = { (Mesh::BaseVertex*)ptr, size };
    ptr += size * sizeof(Mesh::BaseVertex);

    // Vertices
    util_assert(compiled_mesh.size() > header->offset_to_vertices, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_vertices;

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_vertex_data_view.m_vertices = { (Mesh::Vertex*)ptr, size };
    ptr += size * sizeof(Mesh::Vertex);

    // Indices
    util_assert(compiled_mesh.size() > header->offset_to_indices, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_indices;

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_vertex_data_view.m_indices = { (u32*)ptr, size };
    ptr += size * sizeof(u32);

    // Textures
    util_assert(compiled_mesh.size() > header->offset_to_textures, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_textures;

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_texture_data.m_texture_memory.resize(size);
    for (u64 i = 0; i < size; i++) {
        auto& texture_memory = mesh.m_texture_data.m_texture_memory.at(i);
        ptr = texture_memory.deserialize(ptr);
        texture_memory.owned = false;
    }

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_texture_data.m_diffuse_textures_memory.resize(size);
    memcpy(mesh.m_texture_data.m_diffuse_textures_memory.data(), ptr, size * sizeof(u32));
    ptr += size * sizeof(u32);

    mesh.m_texture_data.m_metallic_roughness_textures_memory.resize(size);
    memcpy(mesh.m_texture_data.m_metallic_roughness_textures_memory.data(), ptr, size * sizeof(u32));
    ptr += size * sizeof(u32);

    mesh.m_texture_data.m_normal_textures_memory.resize(size);
    memcpy(mesh.m_texture_data.m_normal_textures_memory.data(), ptr, size * sizeof(u32));
    ptr += size * sizeof(u32);

    // Bones
    util_assert(compiled_mesh.size() > header->offset_to_bones, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_bones;
    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);
    mesh.m_has_bones = (size != 0U);

    mesh.m_vertex_data_view.m_bones = { (VertexBone*)ptr, size };
    ptr += size * sizeof(VertexBone);

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    for (u64 i = 0; i < size; i++) {
        BoneNameID bone;
        ptr = bone.deserialize(ptr);
        mesh.m_bone_id_map[bone.name] = bone.id;
    }

    // Animations
    util_assert(compiled_mesh.size() > header->offset_to_animations, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_animations;

    std::memcpy(&size, ptr, sizeof(u64));
    ptr += sizeof(size);

    mesh.m_animations.resize(size);
    for (u64 i = 0; i < size; i++) {
        ptr = mesh.m_animations[i].deserialize(ptr);
    }

    // AABB
    util_assert(compiled_mesh.size() > header->offset_to_aabb, "CMESH invalid size");
    ptr = compiled_mesh.data() + header->offset_to_aabb;
    std::memcpy(&mesh.m_aabb, ptr, sizeof(mesh.m_aabb));

    // Upload textures to the GPU
    mesh.upload_texture_memory_to_gpu();
    mesh.drop_texture_memory();

    mesh.setup_mesh();
}

GlobalHeader* MeshCompiler::get_global_header()
{
    return (GlobalHeader*)m_bytes->data();
}

} // namespace Renderer
