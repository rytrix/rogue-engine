#include "mesh.hpp"

#include "model.hpp"

#include "../app_data.hpp"

namespace Renderer {

VertexBone::VertexBone()
{
    for (auto& bone : bones) {
        // bone = UINT32_MAX;
        bone = 0;
    }
}

void VertexBone::add_bone(const u32 bone_id, const float weight)
{
    for (u32 i = 0; i < MAX_BONES_PER_VERTEX; i++) {
        if (bones.at(i) == bone_id) {
            return;
        }
    }
    for (u32 i = 0; i < MAX_BONES_PER_VERTEX; i++) {
        if (weights.at(i) == 0.0F) {
            // if (bones.at(i) == UINT32_MAX) {
            bones.at(i) = bone_id;
            weights.at(i) = weight;
            return;
        }
    }
    util_error("Exceded max number of bones per vertex");
}

Mesh::Mesh(const char* path, GlobalAppData* app_data)
{
    load_mesh(*this, path, app_data);
}

Mesh::~Mesh()
{
    initialized = false;
    drop_texture_memory();
}

void Mesh::update(u32 instance_count, const std::span<glm::mat4> transform_matrices, const std::span<AnimationData*> animation_data)
{
    util_assert(initialized == true, "not initialized");
    assert_result_checked();

    next_ssbo_frame();
    update_instance_count(instance_count);
    update_model_ssbos(transform_matrices);
    if (m_has_bones) {
        update_bone_matrices(animation_data);
    }
}

void Mesh::update_instance_count(u32 instance_count)
{
    util_assert(initialized == true, "not initialized");

    if (m_instance_count != instance_count) {
        m_instance_count = instance_count;

        for (usize i = 0; i < m_commands.size(); i++) {
            m_commands.at(i).instance_count = m_instance_count;
        }

        if (Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture")) {
            m_cmd_buff.buffer_sub_data(0, m_commands.size() * sizeof(m_commands[0]), m_commands.data());
        }

        m_model_ssbo.deinit();
        m_model_ssbo.init(3, m_instance_count * sizeof(glm::mat4));

        m_bone_ssbo.deinit();
        m_bone_ssbo.init(3, m_instance_count * sizeof(glm::mat4) * MAX_BONES);
    }
}

void Mesh::update_model_ssbos(const std::span<glm::mat4> model_matrices)
{
    util_assert(initialized == true, "not initialized");

    void* ptr = m_model_ssbo.get_ptr();
    std::memcpy(ptr, model_matrices.data(), model_matrices.size() * sizeof(glm::mat4));
}

void Mesh::update_bone_matrices(std::span<AnimationData*> animation_data)
{
    util_assert(initialized == true, "not initialized");
    util_assert(m_has_bones == true, "Attempting to update bone matrices with no bones");

    GLsizeiptr offset = 0;

    void* ptr = m_bone_ssbo.get_ptr();
    for (auto* anim : animation_data) {
        auto* current_animation = anim->data[anim->selected_animation];
        std::memcpy((void*)((char*)ptr + offset), current_animation->m_final_transforms, current_animation->m_final_transforms_size * sizeof(glm::mat4));
        offset += MAX_BONES * sizeof(glm::mat4);
    }
}

void Mesh::next_ssbo_frame()
{
    if (m_model_ssbo.is_initialized()) {
        m_model_ssbo.increment_frame();
    }
    if (m_bone_ssbo.is_initialized()) {
        m_bone_ssbo.increment_frame();
    }
}

void Mesh::draw_untextured([[maybe_unused]] Renderer::Shader& shader)
{
    util_assert(initialized == true, "not initialized");
    assert_result_checked();

    m_vao.bind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_model_ssbo.get_id());
    if (m_has_bones) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_bone_ssbo.get_id());
    }
    if (Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture")) {
        m_cmd_buff.bind_buffer(GL_DRAW_INDIRECT_BUFFER);

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            m_commands.size(),
            0);

        m_cmd_buff.unbind_buffer(GL_DRAW_INDIRECT_BUFFER);
    } else {
        for (usize i = 0; i < m_commands.size(); i++) {
            glDrawElementsInstancedBaseVertexBaseInstance(
                GL_TRIANGLES,
                m_commands[i].count,
                GL_UNSIGNED_INT,
                (void*)(m_commands[i].first_index * sizeof(GLuint)),
                m_commands[i].instance_count,
                m_commands[i].base_vertex,
                m_commands[i].base_instance);
        }
    }
}

void Mesh::draw(Shader& shader)
{
    util_assert(initialized == true, "not initialized");
    assert_result_checked();

    m_vao.bind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_model_ssbo.get_id());
    if (m_has_bones) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_bone_ssbo.get_id());
    }
    if (Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture")) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_texture_ssbo.get_id());

        m_cmd_buff.bind_buffer(GL_DRAW_INDIRECT_BUFFER);

        glMultiDrawElementsIndirect(
            GL_TRIANGLES,
            GL_UNSIGNED_INT,
            nullptr,
            m_commands.size(),
            0);

        m_cmd_buff.unbind_buffer(GL_DRAW_INDIRECT_BUFFER);
    } else {
        for (usize i = 0; i < m_commands.size(); i++) {
            GLuint texture_unit = Texture::get_texture_unit();
            auto* diffuse_texture = m_app_data->m_texture_cache.get(m_texture_data.m_diffuse_textures[i]);
            diffuse_texture->bind(texture_unit);
            shader.set_int("tex_diffuse", static_cast<int>(texture_unit));

            texture_unit = Texture::get_texture_unit();
            auto* metallic_roughness_texture = m_app_data->m_texture_cache.get(m_texture_data.m_metallic_roughness_textures[i]);
            metallic_roughness_texture->bind(texture_unit);
            shader.set_int("tex_metallic_roughness", static_cast<int>(texture_unit));

            texture_unit = Texture::get_texture_unit();
            auto* normal_texture = m_app_data->m_texture_cache.get(m_texture_data.m_normal_textures[i]);
            normal_texture->bind(texture_unit);
            shader.set_int("tex_normals", static_cast<int>(texture_unit));

            glDrawElementsInstancedBaseVertexBaseInstance(
                GL_TRIANGLES,
                m_commands[i].count,
                GL_UNSIGNED_INT,
                (void*)(m_commands[i].first_index * sizeof(GLuint)),
                m_commands[i].instance_count,
                m_commands[i].base_vertex,
                m_commands[i].base_instance);

            Texture::drop_texture_units(3);
        }
    }
}

ModelResult Mesh::get_result()
{
    m_result_checked = true;
    return m_result;
}

void Mesh::upload_texture_memory_to_gpu()
{
    std::vector<Handle> handles;
    handles.resize(m_texture_data.m_texture_memory.size());

    for (usize i = 0; i < m_texture_data.m_texture_memory.size(); i++) {
        TextureInfo info {};
        info.texture_loaded = true;
        info.texture_memory = m_texture_data.m_texture_memory[i];
        info.min_filter = GL_LINEAR_MIPMAP_LINEAR;
        info.mag_filter = GL_LINEAR;
        info.mipmaps = true;
        info.mipmap_levels = 0;

        auto handle = m_app_data->m_texture_cache.create(info);
        auto* texture = m_app_data->m_texture_cache.get(handle);
        texture->set_max_anisotropy(16);
        handles[i] = handle;
    }

    for (usize i = 0; i < m_texture_data.m_diffuse_textures_memory.size(); i++) {
        u32 index = m_texture_data.m_diffuse_textures_memory[i];
        if (index == UINT32_MAX) {
            m_texture_data.m_diffuse_textures.emplace_back(m_app_data->m_default_textures.get_albedo());
        } else {
            m_texture_data.m_diffuse_textures.emplace_back(handles[index]);
        }

        index = m_texture_data.m_metallic_roughness_textures_memory[i];
        if (index == UINT32_MAX) {
            m_texture_data.m_metallic_roughness_textures.emplace_back(m_app_data->m_default_textures.get_metallic());
        } else {
            m_texture_data.m_metallic_roughness_textures.emplace_back(handles[index]);
        }

        index = m_texture_data.m_normal_textures_memory[i];
        if (index == UINT32_MAX) {
            m_texture_data.m_normal_textures.emplace_back(m_app_data->m_default_textures.get_normal());
        } else {
            m_texture_data.m_normal_textures.emplace_back(handles[index]);
        }
    }
}

void Mesh::drop_texture_memory()
{
    for (usize i = 0; i < m_texture_data.m_texture_memory.size(); i++) {
        m_texture_data.m_texture_memory[i].deinit();
    }

    m_texture_data.m_texture_memory.clear();
    m_texture_data.m_diffuse_textures_memory.clear();
    m_texture_data.m_metallic_roughness_textures_memory.clear();
    m_texture_data.m_normal_textures_memory.clear();
}

void Mesh::setup_mesh()
{
    util_assert(initialized == false, "already initialized");

    m_vao.init();
    m_vbo.init();
    m_ebo.init();

    m_vao.bind();

    m_vbo.buffer_data(static_cast<i64>(m_vertex_data_view.m_vertices.size() * sizeof(Vertex)), m_vertex_data_view.m_vertices.data(), GL_STATIC_DRAW);
    m_ebo.buffer_data(static_cast<i64>(m_vertex_data_view.m_indices.size() * sizeof(u32)), m_vertex_data_view.m_indices.data(), GL_STATIC_DRAW);

    m_vao.bind_vertex_buffer(0, m_vbo.get_id(), 0, sizeof(Vertex));
    m_vao.bind_element_buffer(m_ebo.get_id());

    m_vao.vertex_attrib(0, 0, 3, GL_FLOAT, 0);
    m_vao.vertex_attrib(1, 0, 3, GL_FLOAT, offsetof(Vertex, m_norm));
    m_vao.vertex_attrib(2, 0, 2, GL_FLOAT, offsetof(Vertex, m_tex));
    m_vao.vertex_attrib(3, 0, 3, GL_FLOAT, offsetof(Vertex, m_tang));

    if (m_has_bones) {
        m_bones_vbo.init();
        m_bones_vbo.buffer_data(static_cast<i64>(m_vertex_data_view.m_bones.size() * sizeof(VertexBone)), m_vertex_data_view.m_bones.data(), GL_STATIC_DRAW);
        m_vao.bind_vertex_buffer(1, m_bones_vbo.get_id(), 0, sizeof(VertexBone));

        m_vao.vertex_attrib_int(4, 1, MAX_BONES_PER_VERTEX, GL_INT, 0);
        m_vao.vertex_attrib(5, 1, MAX_BONES_PER_VERTEX, GL_FLOAT, MAX_BONES_PER_VERTEX * sizeof(GLint));
    }

    m_commands.resize(m_vertex_data_view.m_base_vertices.size());

    for (usize i = 0; i < m_vertex_data_view.m_base_vertices.size(); i++) {
        m_commands[i].count = m_vertex_data_view.m_base_vertices[i].m_count;
        m_commands[i].instance_count = m_instance_count;
        m_commands[i].first_index = m_vertex_data_view.m_base_vertices[i].m_offset; // * sizeof(GLuint);
        m_commands[i].base_instance = 0;
        m_commands[i].base_vertex = m_vertex_data_view.m_base_vertices[i].m_base;
    }

    if (Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture")) {
        m_cmd_buff.init();
        m_cmd_buff.buffer_storage(m_commands.size() * sizeof(IndirectCommands), m_commands.data(), GL_DYNAMIC_STORAGE_BIT);

        m_texture_ssbo.init();
        m_texture_bindless_ids.resize(m_commands.size() * 3);
        for (usize i = 0; i < m_commands.size(); i++) {
            // 1 diffuse 1 metallic_roughness 1 normal
            auto* diffuse_texture = m_app_data->m_texture_cache.get(m_texture_data.m_diffuse_textures[i]);
            m_texture_bindless_ids.at((i * 3) + 0) = diffuse_texture->get_bindless_texture_id();
            if (!diffuse_texture->is_bindless_texture_mapped()) {
                diffuse_texture->map_bindless_texture();
            }

            auto* metallic_roughness_texture = m_app_data->m_texture_cache.get(m_texture_data.m_metallic_roughness_textures[i]);
            m_texture_bindless_ids.at((i * 3) + 1) = metallic_roughness_texture->get_bindless_texture_id();
            if (!metallic_roughness_texture->is_bindless_texture_mapped()) {
                metallic_roughness_texture->map_bindless_texture();
            }

            auto* normal_texture = m_app_data->m_texture_cache.get(m_texture_data.m_normal_textures[i]);
            m_texture_bindless_ids.at((i * 3) + 2) = normal_texture->get_bindless_texture_id();
            if (!normal_texture->is_bindless_texture_mapped()) {
                normal_texture->map_bindless_texture();
            }
        }

        if (m_texture_bindless_ids.size() > 0) {
            m_texture_ssbo.buffer_storage(m_texture_bindless_ids.size() * sizeof(GLuint64), m_texture_bindless_ids.data(), 0);
        }
    }

    if (m_has_bones) {
        m_bone_ssbo.init(3, m_instance_count * sizeof(glm::mat4) * MAX_BONES);
    }

    m_model_ssbo.init(3, m_instance_count * sizeof(glm::mat4));

    initialized = true;
}

void Mesh::assert_result_checked()
{
    if constexpr (Utils::runtime_checks) {
        util_assert(m_result_checked == true, "did not check mesh result");
    }
}

} // namespace Renderer
