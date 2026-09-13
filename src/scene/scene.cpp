#include "scene.hpp"

#include "../physics_jolt/helpers.hpp"
#include "../physics_jolt/interface.hpp"

#include "../utils/assert.hpp"
#include "../utils/file.hpp"

#include "glm/gtc/quaternion.hpp"

#include "entity.hpp"
#include "transform.hpp"

#include "../app_data.hpp"

namespace {

constexpr void get_pbr_forward_pass_indirect(ShaderInfoData<2>& out, const std::string& light_uniforms, const std::string& light_functions, const std::string& vert_defines, const std::string& frag_defines)
{

    const char* file_name = "res/shaders/forward_pass/pbr_combined.glsl";
    std::vector<char> pbr_file;
    auto result = Utils::read_file(pbr_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view pbr_file_view = { pbr_file.data(), pbr_file.size() };

    // Vertex Shader
    out.data.at(0) = std::format("#version 460 core\n#define SSBO0\n");
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(pbr_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n#define BINDLESS_TEXTURES\n";
    out.data.at(1) += frag_defines;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// Fragment Begin", "// Light Uniforms Begin");
    out.data.at(1) += light_uniforms;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// Light Uniforms End", "// LO Functions Begin");
    out.data.at(1) += light_functions;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// LO Functions End", "// Fragment End");

    out.populate_info();
}

constexpr void get_pbr_forward_pass_normal(ShaderInfoData<2>& out, const std::string& light_uniforms, const std::string& light_functions, const std::string& vert_defines, const std::string& frag_defines)
{
    const char* file_name = "res/shaders/forward_pass/pbr_combined.glsl";
    std::vector<char> pbr_file;
    auto result = Utils::read_file(pbr_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view pbr_file_view = { pbr_file.data(), pbr_file.size() };

    // Vertex Shader
    out.data.at(0) = std::format("#version 460 core\n#define SSBO0\n");
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(pbr_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n#define UNIFORM_TEXTURES\n";
    out.data.at(1) += frag_defines;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// Fragment Begin", "// Light Uniforms Begin");
    out.data.at(1) += light_uniforms;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// Light Uniforms End", "// LO Functions Begin");
    out.data.at(1) += light_functions;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// LO Functions End", "// Fragment End");

    out.populate_info();
}

constexpr void get_shadow_pass_basic_shaders(ShaderInfoData<2>& out, const std::string& vert_defines, const std::string& frag_defines)
{

    const char* file_name = "res/shaders/forward_pass/shadow_pass_basic.glsl";
    std::vector<char> shadow_file;
    auto result = Utils::read_file(shadow_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view shadow_file_view = { shadow_file.data(), shadow_file.size() };

    // Vertex Shader
    out.data.at(0) = "#version 460 core\n";
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(shadow_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n";
    out.data.at(1) += frag_defines;
    out.data.at(1) += get_lines_between_delims(shadow_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();
}

constexpr void get_shadow_pass_point_shaders(ShaderInfoData<2>& out, const std::string& vert_defines, const std::string& frag_defines)
{
    const char* file_name = "res/shaders/forward_pass/shadow_pass_point.glsl";
    std::vector<char> shadow_file;
    auto result = Utils::read_file(shadow_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view shadow_file_view = { shadow_file.data(), shadow_file.size() };

    // Vertex Shader
    out.data.at(0) = "#version 460 core\n";
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(shadow_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n";
    out.data.at(1) += frag_defines;
    out.data.at(1) += get_lines_between_delims(shadow_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();
}

constexpr void get_shadow_pass_point_geometry_shaders(ShaderInfoData<3>& out, const std::string& vert_defines, const std::string& frag_defines)
{
    const char* file_name = "res/shaders/forward_pass/shadow_pass_point_geometry.glsl";
    std::vector<char> shadow_file;
    auto result = Utils::read_file(shadow_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view shadow_file_view = { shadow_file.data(), shadow_file.size() };

    // Vertex Shader
    out.data.at(0) = "#version 460 core\n";
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(shadow_file_view, "// Vertex Begin", "// Vertex End");

    // Geometry Shader
    out.data.at(1) = "#version 460 core\n";
    out.data.at(1) += get_lines_between_delims(shadow_file_view, "// Geometry Begin", "// Geometry End");

    // Fragment Shader
    out.data.at(2) += "#version 460 core\n";
    out.data.at(2) += frag_defines;
    out.data.at(2) += get_lines_between_delims(shadow_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();
}

constexpr void get_wireframe_shaders(ShaderInfoData<2>& out, const std::string& vert_defines, const std::string& frag_defines)
{
    const char* file_name = "res/shaders/forward_pass/static_lines.glsl";
    std::vector<char> pbr_file;
    auto result = Utils::read_file(pbr_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view pbr_file_view = { pbr_file.data(), pbr_file.size() };

    // Vertex Shader
    out.data.at(0) = std::format("#version 460 core\n");
    out.data.at(0) += vert_defines;
    out.data.at(0) += get_lines_between_delims(pbr_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n";
    out.data.at(1) += frag_defines;
    out.data.at(1) += get_lines_between_delims(pbr_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();
}

} // anonymous namespace

Scene::Scene(GlobalAppData* app_data)
    : m_app_data(app_data)
    , m_random_sampling_texture(Renderer::RandomSamplingTexture::create(16, 8, 2, &app_data->m_texture_cache))
{
    m_physics_system = std::make_unique<Physics::System>(this, app_data);
}

Scene::~Scene()
{
    auto view = m_registry.view<Physics::PhysicsInfo>();
    for (auto [entity, body] : view.each()) {
        m_physics_system->m_body_interface->RemoveBody(body.m_id);
        m_physics_system->m_body_interface->DestroyBody(body.m_id);
    }
}

Entity Scene::create_entity()
{
    Entity entity { this, m_registry.create() };

    Utils::String name;
    name.format("default_{}", (size_t)entity.get_id());
    Entity::add_name(entity, name.c_str());
    Transform transform;
    Entity::add_transform(entity, transform);

    return entity;
}

void Scene::remove_entity(Entity entity)
{
    m_registry.destroy(entity.get_id());
}

Entity Scene::get_entity_by_name(const char* name)
{
    auto view = m_registry.view<Utils::String>();

    for (auto [entity, e_name] : view.each()) {
        if (e_name == name) {
            return { this, entity };
        }
    }

    return { this, entt::null };
}

void Scene::to_json(nlohmann::json& json)
{
    auto view = m_registry.view<entt::entity>();
    for (auto [entt_entity] : view.each()) {
        Entity entity(this, entt_entity);
        nlohmann::json json_entity;
        Entity::to_json(json_entity, entity);

        // std::println("json_entity: {}", json_entity.dump());

        json["entities"].emplace_back(json_entity);
    }
}

void Scene::from_json(nlohmann::json& json)
{
    for (auto& json_entity : json["entities"]) {
        auto entity = create_entity();
        Entity::from_json(json_entity, entity);
    }
}

void Scene::update()
{
    if (m_first_update) {
        m_clock.reset();
        m_first_update = false;
    }

    m_clock.update();

    if (m_physics_needs_optimize) {
        m_physics_system->optimize();
        m_physics_needs_optimize = false;
    }

    if (m_physics_on) {
        m_physics_system->update(m_clock.delta_time<float>());

        auto view = m_registry.view<Transform, Physics::PhysicsInfo>();

        for (auto [entity, transform, body] : view.each()) {
            if (body.m_motion_type != JPH::EMotionType::Static) {
                auto& model = transform.get_model_matrix_ref();

                // glm::mat4 physics_model = Physics::mat4_to_mat4(m_physics_system->m_body_interface->GetCenterOfMassTransform(body.m_id));

                glm::vec3 position = Physics::vec3_to_vec3(m_physics_system->m_body_interface->GetPosition(body.m_id));
                glm::quat rotation = Physics::quat_to_quat(m_physics_system->m_body_interface->GetRotation(body.m_id));

                transform.set_position(position);
                transform.set_rotation(rotation);

                auto* point_light = m_registry.try_get<Renderer::Light::Pbr::Point>(entity);
                if (point_light != nullptr) {
                    point_light->position = model[3];
                }

                auto* spot_light = m_registry.try_get<Renderer::Light::Pbr::Spot>(entity);
                if (spot_light != nullptr) {
                    spot_light->position = model[3];
                }
            }
        }
    }

    compile_shaders();
}

void Scene::draw()
{
    if (m_mesh_instance_draw_cache_needs_update) {
        auto mesh_view = m_registry.view<Transform, Renderer::Mesh*>();

        m_mesh_instance_draw_cache.clear();
        for (auto [entity, transform, mesh] : mesh_view.each()) {

            bool contained = false;
            for (auto& mesh_cached : m_mesh_instance_draw_cache) {
                if (mesh == mesh_cached.m_mesh) {
                    contained = true;
                }
            }

            if (!contained) {
                m_mesh_instance_draw_cache.emplace_back(mesh, transform.get_model_matrix());
            }

            for (auto& model_cached : m_mesh_instance_draw_cache) {
                if (mesh == model_cached.m_mesh) {
                    model_cached.m_transform_matrices.emplace_back(transform.get_model_matrix());

                    if (model_cached.m_mesh->m_has_bones) {
                        auto& data = m_registry.get<Renderer::AnimationData>(entity);
                        model_cached.m_animation_data.emplace_back(&data);
                    }
                }
            }
        }

        LOG_INFO("Updated scene instanced draw cache");
        m_mesh_instance_draw_cache_needs_update = false;
    } else {
        auto mesh_view = m_registry.view<Transform, Renderer::Mesh*>();

        for (auto& mesh : m_mesh_instance_draw_cache) {
            mesh.m_transform_matrices.clear();
        }
        for (auto [entity, transform, mesh] : mesh_view.each()) {
            for (usize j = 0; j < m_mesh_instance_draw_cache.size(); j++) {
                if (mesh == m_mesh_instance_draw_cache[j].m_mesh) {
                    m_mesh_instance_draw_cache[j].m_transform_matrices.emplace_back(transform.get_model_matrix());
                }
            }
        }
    }

    for (auto& mesh : m_mesh_instance_draw_cache) {
        for (auto* animation_data : mesh.m_animation_data) {
            auto* current_animation = animation_data->data[animation_data->selected_animation];
            Renderer::PerAnimationData* second_animation = nullptr;
            if (animation_data->second_animation != UINT32_MAX) {
                second_animation = animation_data->data[animation_data->second_animation];
            }
            if (animation_data->paused == false) {
                current_animation->m_animation_time += m_clock.delta_time<float>();
                if (second_animation != nullptr) {
                    second_animation->m_animation_time += m_clock.delta_time<float>();

                    const f32 BLEND_SPEED = 0.5F;
                    animation_data->blend_factor += m_clock.delta_time<float>() * BLEND_SPEED;

                    float factor = 1.0F - animation_data->blend_factor;
                    current_animation->update_transforms_blended(second_animation, factor);
                    if (animation_data->blend_factor >= 1.0F) {
                        animation_data->second_animation = UINT32_MAX;
                    }
                } else {
                    current_animation->update_transforms();
                }
            }

            // animation_data->m_animation_time += animation_time;
        }
        mesh.m_mesh->update(mesh.m_transform_matrices.size(), mesh.m_transform_matrices, mesh.m_animation_data);
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.0F, 0.0F, 0.0F, 1.0F);

    m_app_data->m_camera.update();

    glCullFace(GL_FRONT);
    auto directional_shadow_view = m_registry.view<Renderer::Light::Pbr::Directional, Renderer::Light::Pbr::DirectionalShadow>();
    for (auto [entity, light, shadow] : directional_shadow_view.each()) {
        shadow.update(light, m_app_data->m_camera);
        shadow.shadowmap_begin();
        for (auto& mesh : m_mesh_instance_draw_cache) {
            shadow.shadowmap_draw(mesh.m_mesh);
        }
        shadow.shadowmap_end();
    }

    auto point_shadow_view = m_registry.view<Renderer::Light::Pbr::Point, Renderer::Light::Pbr::PointShadow>();
    for (auto [entity, light, shadow] : point_shadow_view.each()) {
        shadow.update(light);
        shadow.shadowmap_begin();
        for (auto& mesh_instance : m_mesh_instance_draw_cache) {
            Renderer::Shader& shader = mesh_instance.m_mesh->m_has_bones ? m_shadowmap_cubemap_shader_bones : m_shadowmap_cubemap_shader;
            shadow.shadowmap_draw(shader, light, mesh_instance.m_mesh);
        }
        shadow.shadowmap_end();
    }

    auto spot_shadow_view = m_registry.view<Renderer::Light::Pbr::Spot, Renderer::Light::Pbr::SpotShadow>();
    for (auto [entity, light, shadow] : spot_shadow_view.each()) {
        shadow.update(light);
        shadow.shadowmap_begin();
        for (auto& mesh_instance : m_mesh_instance_draw_cache) {
            Renderer::Shader& shader = mesh_instance.m_mesh->m_has_bones ? m_shadowmap_shader_bones : m_shadowmap_shader;
            shadow.shadowmap_draw(shader, mesh_instance.m_mesh);
        }
        shadow.shadowmap_end();
    }

    glCullFace(GL_BACK);
    glViewport(0, 0, m_app_data->m_window.get_width(), m_app_data->m_window.get_height());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& mesh_instance : m_mesh_instance_draw_cache) {
        Renderer::Shader& shader = mesh_instance.m_mesh->m_has_bones ? m_shader_bones : m_shader;
        shader.bind();
        shader.set_mat4("proj", m_app_data->m_camera.get_proj());
        shader.set_mat4("view", m_app_data->m_camera.get_view());
        shader.set_vec3("view_position", m_app_data->m_camera.get_pos());

        auto pbr_directional_view = m_registry.view<Renderer::Light::Pbr::Directional>();
        auto pbr_point_view = m_registry.view<Renderer::Light::Pbr::Point>();
        auto pbr_spot_view = m_registry.view<Renderer::Light::Pbr::Spot>();
        u32 i = 0;
        for (auto [entity, light] : pbr_directional_view.each()) {
            light.set_uniforms(shader, std::format("u_directional_light_{}", i).c_str());
            auto* shadow = m_registry.try_get<Renderer::Light::Pbr::DirectionalShadow>(entity);
            if (shadow != nullptr) {
                shadow->set_uniforms(shader, std::format("u_directional_light_shadow_{}", i).c_str());
            }
            i++;
        }
        i = 0;
        for (auto [entity, light] : pbr_point_view.each()) {
            light.set_uniforms(shader, std::format("u_point_light_{}", i).c_str());
            auto* shadow = m_registry.try_get<Renderer::Light::Pbr::PointShadow>(entity);
            if (shadow != nullptr) {
                shadow->set_uniforms(shader, std::format("u_point_light_shadow_{}", i).c_str());
            }
            i++;
        }
        i = 0;
        for (auto [entity, light] : pbr_spot_view.each()) {
            light.set_uniforms(shader, std::format("u_spot_light_{}", i).c_str());
            auto* shadow = m_registry.try_get<Renderer::Light::Pbr::SpotShadow>(entity);
            if (shadow != nullptr) {
                shadow->set_uniforms(shader, std::format("u_spot_light_shadow_{}", i).c_str());
            }
            i++;
        }

        m_random_sampling_texture.bind_uniforms(shader, "tex_random_offset", &m_app_data->m_texture_cache);

        mesh_instance.m_mesh->draw(shader);

        Renderer::Texture::drop_texture_units(1);
    }

    if (has_component<Renderer::Skybox>()) {
        auto& skybox = get_component<Renderer::Skybox>();
        skybox.draw(m_app_data->m_camera);
    }

    Renderer::Texture::reset_texture_units();
}

void Scene::draw_entity_wireframe(Entity entity, glm::vec4 color)
{
    if (!entity.valid()) {
        LOG_ERROR("Invalid entity provided to draw_entity_wireframe");
        return;
    }
    auto transform = entity.get_component<Transform>();
    auto* mesh = entity.get_component<Renderer::Mesh*>();

    glDisable(GL_DEPTH_TEST);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    std::array<glm::mat4, 1> transform_temp { transform.get_model_matrix() };
    if (mesh->m_has_bones) {
        auto& animation_data = entity.get_component<Renderer::AnimationData>();
        std::array<Renderer::AnimationData*, 1> animation_data_temp { &animation_data };
        mesh->update(1, transform_temp, animation_data_temp);

        m_wireframe_shader_bones.bind();
        m_wireframe_shader_bones.set_mat4("proj", m_app_data->m_camera.get_proj());
        m_wireframe_shader_bones.set_mat4("view", m_app_data->m_camera.get_view());
        m_wireframe_shader_bones.set_vec4("u_color", color);
        mesh->draw_untextured(m_wireframe_shader_bones);
    } else {
        mesh->update(1, transform_temp, {});

        m_wireframe_shader.bind();
        m_wireframe_shader.set_mat4("proj", m_app_data->m_camera.get_proj());
        m_wireframe_shader.set_mat4("view", m_app_data->m_camera.get_view());
        m_wireframe_shader.set_vec4("u_color", color);
        mesh->draw_untextured(m_wireframe_shader);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_DEPTH_TEST);
}

void Scene::draw_debug_imgui()
{
    if (ImGui::Button("Reload shaders")) {
        m_shaders_need_update = true;
    }

    if (ImGui::Button("Save scene")) {
        nlohmann::json scene;
        to_json(scene);

        auto text = scene.dump(2);
        Utils::String file_name;
        file_name.format("{}.json", m_name.c_str());
        std::ofstream file(file_name.c_str());

        file << text;
    }

    glm::vec3 cam_pos = m_app_data->m_camera.get_pos();
    ImGui::Text("%s", std::format("Camera Pos: {}, {}, {}", cam_pos.x, cam_pos.y, cam_pos.z).c_str());

    float camera_speed = m_app_data->m_camera.get_speed();
    if (ImGui::DragFloat("Camera Speed", &camera_speed, 0.1F, 1.0F, 50.0F)) {
        m_app_data->m_camera.set_speed(camera_speed);
    }

    i32 i = 0;
    if (ImGui::CollapsingHeader("Entities")) {
        if (ImGui::Button("Add Entity")) {
            create_entity();
        }

        auto view = m_registry.view<entt::entity>();
        for (auto [entity] : view.each()) {
            ImGui::PushID(i);

            Utils::String no_name("no_name");
            Utils::String* name_check = m_registry.try_get<Utils::String>(entity);
            Utils::String& entity_name = name_check == nullptr ? no_name : *name_check;
            auto name = std::format("{} e{}", entity_name.c_str(), i);

            if (m_registry.valid(entity)) {
                ImGui::Text("%s", name.c_str());
                ImGui::SameLine();
                if (ImGui::Button("Select Entity")) {
                    m_app_data->m_entity_selector.select_entity(Entity(this, entity));
                }
            }

            ImGui::PopID();
            i++;
        }
    }
}

const Utils::DeltaTime& Scene::get_clock()
{
    return m_clock;
}

void Scene::compile_shaders()
{
    if (!m_shaders_need_update) {
        return;
    }
    LOG_INFO("Compiling shaders");

    std::string no_defines;
    std::string bone_defines = Renderer::get_bone_defines();

    compile_pbr_shaders(no_defines, bone_defines);

    if (!m_shadowmap_shader.is_initialized()) {
        ShaderInfoData<2> shadowmap_shaders;
        get_shadow_pass_basic_shaders(shadowmap_shaders, no_defines, no_defines);
        m_shadowmap_shader.init(shadowmap_shaders.info.data(), shadowmap_shaders.info.size());
    }

    if (!m_shadowmap_shader_bones.is_initialized()) {
        ShaderInfoData<2> shadowmap_shaders;
        get_shadow_pass_basic_shaders(shadowmap_shaders, bone_defines, no_defines);
        m_shadowmap_shader_bones.init(shadowmap_shaders.info.data(), shadowmap_shaders.info.size());
    }

    if (!m_shadowmap_cubemap_shader.is_initialized()) {
        if constexpr (!Renderer::Light::Pbr::PointShadow::USE_GEOMETRY_SHADER) {
            ShaderInfoData<2> cubemap_shaders;
            get_shadow_pass_point_shaders(cubemap_shaders, no_defines, no_defines);
            m_shadowmap_cubemap_shader.init(cubemap_shaders.info.data(), cubemap_shaders.info.size());
        } else {
            ShaderInfoData<3> cubemap_shaders;
            get_shadow_pass_point_geometry_shaders(cubemap_shaders, no_defines, no_defines);
            m_shadowmap_cubemap_shader.init(cubemap_shaders.info.data(), cubemap_shaders.info.size());
        }
    }

    if (!m_shadowmap_cubemap_shader_bones.is_initialized()) {
        if constexpr (!Renderer::Light::Pbr::PointShadow::USE_GEOMETRY_SHADER) {
            ShaderInfoData<2> cubemap_shaders;
            get_shadow_pass_point_shaders(cubemap_shaders, bone_defines, no_defines);
            m_shadowmap_cubemap_shader_bones.init(cubemap_shaders.info.data(), cubemap_shaders.info.size());
        } else {
            ShaderInfoData<3> cubemap_shaders;
            get_shadow_pass_point_geometry_shaders(cubemap_shaders, bone_defines, no_defines);
            m_shadowmap_cubemap_shader_bones.init(cubemap_shaders.info.data(), cubemap_shaders.info.size());
        }
    }

    if (!m_wireframe_shader.is_initialized()) {
        ShaderInfoData<2> line_shaders;
        get_wireframe_shaders(line_shaders, "#define SSBO0\n", "");
        m_wireframe_shader.init(line_shaders.info.data(), line_shaders.info.size());
    }

    if (!m_wireframe_shader_bones.is_initialized()) {
        ShaderInfoData<2> line_shaders_bones;
        get_wireframe_shaders(line_shaders_bones, "#define SSBO0\n" + bone_defines, "");
        m_wireframe_shader_bones.init(line_shaders_bones.info.data(), line_shaders_bones.info.size());
    }

    m_shaders_need_update = false;
}

void Scene::compile_pbr_shaders(const std::string& empty_defines, const std::string& bone_defines)
{
    std::string light_uniforms;
    std::string light_functions;

    u32 i = 0;
    auto directional_view = m_registry.view<Renderer::Light::Pbr::Directional>();
    for (auto [entity, light] : directional_view.each()) {
        light_uniforms += std::format("uniform DirectionalLight u_directional_light_{};\n", i);
        auto* shadow = m_registry.try_get<Renderer::Light::Pbr::DirectionalShadow>(entity);
        if (shadow != nullptr) {
            light_uniforms += std::format("uniform DirectionalLightShadow u_directional_light_shadow_{};\n", i);
            light_functions += std::format("lo += (1.0 - shadow_calculation_directional(u_directional_light_shadow_{}, bias)) * pbr_directional(u_directional_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i, i);
        } else {
            light_functions += std::format("lo += pbr_directional(u_directional_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i);
        }
        i++;
    }
    auto point_view = m_registry.view<Renderer::Light::Pbr::Point>();
    i = 0;
    for (auto [entity, light] : point_view.each()) {
        light_uniforms += std::format("uniform PointLight u_point_light_{};\n", i);

        auto* shadow = m_registry.try_get<Renderer::Light::Pbr::PointShadow>(entity);
        if (shadow != nullptr) {
            light_uniforms += std::format("uniform PointLightShadow u_point_light_shadow_{};\n", i);
            light_functions += std::format("lo += (1.0 - shadow_calculation_point(u_point_light_{}, u_point_light_shadow_{}, bias)) * pbr_point(u_point_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i, i, i);
        } else {
            light_functions += std::format("lo += pbr_point(u_point_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i);
        }
        i++;
    }
    auto spot_view = m_registry.view<Renderer::Light::Pbr::Spot>();
    i = 0;
    for (auto [entity, light] : spot_view.each()) {
        light_uniforms += std::format("uniform SpotLight u_spot_light_{};\n", i);

        auto* shadow = m_registry.try_get<Renderer::Light::Pbr::SpotShadow>(entity);
        if (shadow != nullptr) {
            light_uniforms += std::format("uniform SpotLightShadow u_spot_light_shadow_{};\n", i);
            light_functions += std::format("lo += (1.0 - shadow_calculation_spot(u_spot_light_shadow_{}, bias)) * pbr_spot(u_spot_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i, i, i);
        } else {
            light_functions += std::format("lo += pbr_spot(u_spot_light_{}, albedo, roughness, metallic, normal, view, base_reflectivity);", i);
        }
        i++;
    }

    ShaderInfoData<2> shader_source;
    ShaderInfoData<2> shader_source_bones;

    std::string fragment_defines = "#define RANDOM_SAMPLING\n";
    if (Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture")) {
        get_pbr_forward_pass_indirect(shader_source, light_uniforms, light_functions, empty_defines, fragment_defines);
        get_pbr_forward_pass_indirect(shader_source_bones, light_uniforms, light_functions, bone_defines, fragment_defines);
    } else {
        get_pbr_forward_pass_normal(shader_source, light_uniforms, light_functions, empty_defines, fragment_defines);
        get_pbr_forward_pass_normal(shader_source_bones, light_uniforms, light_functions, bone_defines, fragment_defines);
    }

    if (m_shader.is_initialized()) {
        m_shader.deinit();
    }
    m_shader.init(shader_source.info.data(), shader_source.info.size());

    if (m_shader_bones.is_initialized()) {
        m_shader_bones.deinit();
    }
    m_shader_bones.init(shader_source_bones.info.data(), shader_source_bones.info.size());
}
