#include "entity_selector.hpp"

#define ENTITY_IMPL
#include "../entity.hpp"

#include "../../app_data.hpp"

#include "../../utils/color.hpp"

EntitySelector::EntitySelector(Scene* scene)
    : m_scene(scene)
{
}

void EntitySelector::init(Scene* scene)
{
    m_scene = scene;
}

void EntitySelector::on_event(Event& event)
{
    if (event.m_type == Event::Type::SDL) {
        if (event.m_sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.m_sdl_event.button.button == SDL_BUTTON_LEFT) {
                if (!m_selected_entity.valid() && m_hovered_entity.valid()) {
                    select_entity(m_hovered_entity);
                    event.m_consumed = true;
                }
            }
        }

        if (event.m_sdl_event.type == SDL_EVENT_KEY_DOWN) {
            if (event.m_sdl_event.key.key == SDLK_ESCAPE) {
                deselect_entity();
                event.m_consumed = true;
            }
        }
    }
}

void EntitySelector::update()
{
    if (m_hover_enabled && !m_selected_entity.valid() && !g_app_data->m_capture_mouse) {
        auto ray_result = m_scene->m_physics_engine->ray_cast(Utils::ray_from_mouse(), g_app_data->m_camera->get_far());
        if (ray_result.has_value()) {
            auto body_id = ray_result.value();
            auto view = m_scene->m_registry.view<PhysicsBox3d::EntityInfo>();
            for (auto [entity, body] : view.each()) {
                if (B3_ID_EQUALS(body.m_id, body_id)) {
                    m_hovered_entity = Entity(m_scene, entity);
                }
            }
        }
    } else {
        m_hovered_entity = Entity(m_scene, entt::null);
    }

    if (m_selected_entity.valid() && !g_app_data->m_capture_mouse) {
        // Expect physics to be off when an entity is selected, but if it gets set to on,
        // the user expects the physics state to remain consistant, so clicking while an entity
        // is selected will invalidate the previous physics state.
        if (m_scene->m_physics_on) {
            m_prev_physics_state = State::Invalid;
        }
    }
}

void EntitySelector::draw()
{
    if (m_hovered_entity.valid()) {
        m_scene->draw_entity_wireframe(m_hovered_entity, glm::vec4(1.0, 0.0, 0.0, 1.0));
    }

    auto& selected_entity = m_selected_entity;

    if (selected_entity.valid() && selected_entity.has_component<Utils::Transform>()) {
        auto* transform = &selected_entity.get_component<Utils::Transform>();

        g_app_data->m_gizmo->m_transform = transform;
        g_app_data->m_gizmo->update();
        g_app_data->m_gizmo->draw();
    }

    draw_selected_entity_imgui();
    draw_model_prompt_window();
    draw_box_hull_prompt_window();
}

void EntitySelector::select_entity(Entity entity)
{
    util_assert(entity.valid(), "Trying to select an invalid entity");
    m_selected_entity = entity;
    g_app_data->m_gizmo->m_state = Gizmo::State::Translation;

    if (m_selected_entity.valid()) {
        m_prev_physics_state = m_scene->m_physics_on ? State::On : State::Off;
        m_scene->m_physics_on = false;
    }
}

void EntitySelector::deselect_entity()
{
    if (m_selected_entity.valid() && m_prev_physics_state != State::Invalid) {
        m_scene->m_physics_on = m_prev_physics_state == State::On;
    }

    m_selected_entity = {};
}

void EntitySelector::draw_selected_entity_imgui()
{
    if (!m_selected_entity.valid()) {
        return;
    }

    constexpr float MAX_TRANSFORM = 64.0F;
    constexpr float MIN_TRANSFORM = -64.0F;

    constexpr float MAX_ROTATION = 360.0F;
    constexpr float MIN_ROTATION = -360.0F;

    constexpr float MAX_COLOR = 3000.0F;
    constexpr float MIN_COLOR = 0.0F;

    if (!m_selected_entity.has_component<Utils::String>()) {
        Entity::add_name(m_selected_entity, "no name");
    }
    Utils::String& name = m_selected_entity.get_component<Utils::String>();

    ImGui::Begin("Selected Entity");

    if (m_imgui_first_time) {
        ImGui::SetNextWindowSize(ImVec2(800, 600));
        m_imgui_first_time = false;
    }

    if (ImGui::InputText("##EntityNameInput", name.data(), name.capacity())) {
    }

    draw_add_remove_component_imgui();

    if (ImGui::Button("Deselect Entity")) {
        g_app_data->m_entity_selector->deselect_entity();
        goto imgui_end_label;
    }

    if (m_selected_entity.has_component<Renderer::Mesh*>() && m_selected_entity.has_component<Renderer::AnimationData>()) {
        auto* mesh = m_selected_entity.get_component<Renderer::Mesh*>();
        auto& animation_data = m_selected_entity.get_component<Renderer::AnimationData>();

        auto& animations = mesh->m_animations;
        i32 current_animation = static_cast<int>(animation_data.selected_animation);

        ImGui::Text("Animation");
        for (u32 j = 0; j < animations.size(); j++) {
            float total_animation_time = animations[j].get_total_animation_time();
            if ((int)j == current_animation) {
                ImGui::Text("(Selected) Animation: %s, %f ticks", animations[j].m_name.c_str(), total_animation_time);
            } else {
                ImGui::Text("Animation: %s, %f ticks", animations[j].m_name.c_str(), total_animation_time);
            }
        }

        if (ImGui::DragInt("Current Animation", &current_animation, 1.0F, 0, static_cast<int>(animations.size() - 1))) {
            if (current_animation >= static_cast<i32>(animations.size())) {
                current_animation = static_cast<i32>(animations.size() - 1);
            }
            animation_data.second_animation = animation_data.selected_animation;
            animation_data.selected_animation = current_animation;
            animation_data.blend_factor = 0.0F;
            m_selected_entity.get_scene()->m_mesh_instance_draw_cache_needs_update = true;
        }

        ImGui::Checkbox("Pause Animation", &animation_data.paused);

        float ticks_per_second = animations[current_animation].get_ticks_per_second();
        if (ImGui::DragFloat("Ticks per second", &ticks_per_second)) {
            animations[current_animation].set_ticks_per_second(ticks_per_second);
        }
    }

    if (m_selected_entity.has_component<PhysicsBox3d::EntityInfo>()) {
        auto& physics_info = m_selected_entity.get_component<PhysicsBox3d::EntityInfo>();
        auto& body_id = physics_info.m_id;
        auto& motion_type = physics_info.m_motion_type;
        auto* scene = m_selected_entity.get_scene();

        if (motion_type != PhysicsBox3d::MotionType::Static) {
            ImGui::Text("Physics");
            glm::vec3 pos = scene->m_physics_engine->get_body_pos(body_id);
            glm::quat rot = scene->m_physics_engine->get_body_rot(body_id);
            if (ImGui::DragFloat3("XYZ", &pos.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
                scene->m_physics_engine->set_body_transform(body_id, pos, rot);
            }

            glm::vec3 euler_angles = glm::degrees(glm::eulerAngles(rot));
            if (ImGui::DragFloat3("Rotation: XYZ", &euler_angles.x, 1.0F, MIN_ROTATION, MAX_ROTATION)) {
                scene->m_physics_engine->set_body_transform(body_id, pos, glm::quat(glm::radians(euler_angles)));
            }

            ImGui::Checkbox("Show Debug Physics Body Wireframe", &physics_info.m_should_debug_draw);
        } else {
            ImGui::Text("Physics - Static Object");

            auto& transform = m_selected_entity.get_component<Utils::Transform>();
            ImGui::Text("Transform");
            glm::vec3 transform_pos = transform.get_position();
            if (ImGui::DragFloat3("Position: XYZ", &transform_pos.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
                transform.set_position(transform_pos);
            }

            glm::vec3 transform_rot = transform.get_euler_angles();
            if (ImGui::DragFloat3("Rotation: XYZ", &transform_rot.x, 1.0F, MIN_ROTATION, MAX_ROTATION)) {
                transform.set_euler_angles(transform_rot);
            }

            glm::vec3 transform_scale = transform.get_scale();
            if (ImGui::DragFloat3("Scale: XYZ", &transform_scale.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
                transform.set_scale(transform_scale);
            }

            if (ImGui::DragFloat("Scale: All", &transform_scale.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
                transform.set_scale(glm::vec3(transform_scale.x));
            }

            if (ImGui::Button("Recreate static body")) {
                Entity::remove_physics_body(m_selected_entity);
                Entity::add_static_body(m_selected_entity);
            }
        }
    }

    if (m_selected_entity.has_component<Utils::Transform>() && !m_selected_entity.has_component<PhysicsBox3d::EntityInfo>()) {
        auto& transform = m_selected_entity.get_component<Utils::Transform>();

        ImGui::Text("Transform");
        glm::vec3 transform_pos = transform.get_position();
        if (ImGui::DragFloat3("Position: XYZ", &transform_pos.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
            transform.set_position(transform_pos);
        }

        glm::vec3 transform_rot = transform.get_euler_angles();
        if (ImGui::DragFloat3("Rotation: XYZ", &transform_rot.x, 1.0F, MIN_ROTATION, MAX_ROTATION)) {
            transform.set_euler_angles(transform_rot);
        }

        glm::vec3 transform_scale = transform.get_scale();
        if (ImGui::DragFloat3("Scale: XYZ", &transform_scale.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
            transform.set_scale(transform_scale);
        }

        if (ImGui::DragFloat("Scale: All", &transform_scale.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
            transform.set_scale(glm::vec3(transform_scale.x));
        }
    }

    if (m_selected_entity.has_component<Renderer::Light::Pbr::Point>()) {
        auto& point = m_selected_entity.get_component<Renderer::Light::Pbr::Point>();

        ImGui::Text("Point Light");
        ImGui::DragFloat3("XYZ", &point.position.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM);
        ImGui::DragFloat3("RGB", &point.color.x, 10.0F, MIN_COLOR, MAX_COLOR);
    }

    if (m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()) {
        auto& directional = m_selected_entity.get_component<Renderer::Light::Pbr::Directional>();

        ImGui::Text("Directional Light");
        ImGui::DragFloat3("XYZ", &directional.direction.x, 1.0F, -1.0F, 1.0F);
        ImGui::DragFloat3("RGB", &directional.color.x, 10.0F, MIN_COLOR, MAX_COLOR);
    }

    if (m_selected_entity.has_component<Renderer::Light::Pbr::Spot>()) {
        auto& spot = m_selected_entity.get_component<Renderer::Light::Pbr::Spot>();

        ImGui::Text("Spot Light");
        ImGui::DragFloat3("Position XYZ", &spot.position.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM);
        ImGui::DragFloat3("Direction XYZ", &spot.direction.x, 1.0F, -1.0F, 1.0F);
        ImGui::DragFloat3("RGB", &spot.color.x, 10.0F, MIN_COLOR, MAX_COLOR);
        bool inner_cutoff_result = ImGui::DragFloat("inner_cutoff", &spot.inner_cutoff_degrees);
        bool outer_cutoff_result = ImGui::DragFloat("outer_cutoff", &spot.outer_cutoff_degrees);
        if (inner_cutoff_result || outer_cutoff_result) {
            spot.calculate_cutoffs();
        }
    }

imgui_end_label:
    ImGui::End();
}

void EntitySelector::draw_add_remove_component_imgui()
{
    if (ImGui::Button("Add Component")) {
        ImGui::OpenPopup("Add Component Popup");
    }
    ImGui::SameLine();
    if (ImGui::Button("Remove Component")) {
        ImGui::OpenPopup("Remove Component Popup");
    }

    if (ImGui::BeginPopup("Add Component Popup")) {
        // if (ImGui::MenuItem("Name")) {
        // }
        // if (ImGui::MenuItem("Add Transform")) {
        // }
        if (!m_selected_entity.has_component<Renderer::Mesh*>() && ImGui::MenuItem("Add Model")) {
            m_model_prompt.valid = true;
            m_model_prompt.entity = m_selected_entity;
            Utils::String string;
            m_model_prompt.path = string;
        }

        bool has_mesh_and_no_physics = m_selected_entity.has_component<Renderer::Mesh*>()
            && !m_selected_entity.has_component<PhysicsBox3d::EntityInfo>();

        if (has_mesh_and_no_physics && ImGui::MenuItem("Add Static Body")) {
            Entity::add_static_body(m_selected_entity);
            has_mesh_and_no_physics = false;
        }
        if (has_mesh_and_no_physics && ImGui::BeginMenu("Add Dynamic Body")) {
            if (ImGui::MenuItem("Convex Hull Shape")) {
                Entity::add_convex_hull_body(m_selected_entity);
            }
            if (ImGui::MenuItem("Box Hull Shape")) {
                m_box_hull_prompt = {};
                m_box_hull_prompt.valid = true;
                m_box_hull_prompt.entity = m_selected_entity;

                auto& mesh = m_selected_entity.get_component<Renderer::Mesh*>();
                auto& transform = m_selected_entity.get_component<Utils::Transform>();

                auto aabb = mesh->m_aabb;

                glm::vec3 scale = transform.get_scale();
                m_box_hull_prompt.info.extent = ((aabb.max - aabb.min) * 0.5F) * scale;
                m_box_hull_prompt.info.center = ((aabb.min + aabb.max) * 0.5F) * scale;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Add Light")) {
            bool has_no_lights = !m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()
                && !m_selected_entity.has_component<Renderer::Light::Pbr::Point>()
                && !m_selected_entity.has_component<Renderer::Light::Pbr::Spot>();

            bool has_no_other_lights = !m_selected_entity.has_component<Renderer::Light::Pbr::Point>()
                && !m_selected_entity.has_component<Renderer::Light::Pbr::Spot>();

            if (has_no_lights && ImGui::MenuItem("Add Directional Light")) {
                Renderer::Light::Pbr::Directional info {};
                Entity::add_pbr_directional_light(m_selected_entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Directional Light Shadow")) {
                Entity::add_pbr_directional_light_shadow(m_selected_entity);
            }

            has_no_other_lights = !m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()
                && !m_selected_entity.has_component<Renderer::Light::Pbr::Spot>();
            if (has_no_lights && ImGui::MenuItem("Add Point Light")) {
                Renderer::Light::Pbr::Point info {};
                Entity::add_pbr_point_light(m_selected_entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Point Light Shadow")) {
                Entity::add_pbr_point_light_shadow(m_selected_entity);
            }

            has_no_other_lights = !m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()
                && !m_selected_entity.has_component<Renderer::Light::Pbr::Point>();
            if (has_no_lights && ImGui::MenuItem("Add Spot Light")) {
                Renderer::Light::Pbr::Spot info {};
                Entity::add_pbr_spot_light(m_selected_entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Spot Light Shadow")) {
                Entity::add_pbr_spot_light_shadow(m_selected_entity);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Remove Component Popup")) {
        if (m_selected_entity.has_component<Renderer::Mesh*>() && ImGui::MenuItem("Remove Model")) {
            Entity::remove_mesh(m_selected_entity);
        }
        if (m_selected_entity.has_component<PhysicsBox3d::EntityInfo>()) {
            auto& physics_info = m_selected_entity.get_component<PhysicsBox3d::EntityInfo>();

            if (physics_info.m_type == PhysicsBox3d::Type::Mesh && ImGui::MenuItem("Remove Static Body")) {
                Entity::remove_physics_body(m_selected_entity);
            }

            if (physics_info.m_type != PhysicsBox3d::Type::Mesh && ImGui::MenuItem("Remove Dynamic Body")) {
                Entity::remove_physics_body(m_selected_entity);
            }
        }

        bool has_light = m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()
            || m_selected_entity.has_component<Renderer::Light::Pbr::Point>()
            || m_selected_entity.has_component<Renderer::Light::Pbr::Spot>();

        if (has_light && ImGui::BeginMenu("Remove Light")) {
            if (m_selected_entity.has_component<Renderer::Light::Pbr::Directional>()
                && ImGui::MenuItem("Remove Directional Light")) {

                Entity::remove_pbr_directional_light(m_selected_entity);
            }
            if (m_selected_entity.has_component<Renderer::Light::Pbr::DirectionalShadow>()
                && ImGui::MenuItem("Remove Directional Light Shadow")) {

                Entity::remove_pbr_directional_light_shadow(m_selected_entity);
            }
            if (m_selected_entity.has_component<Renderer::Light::Pbr::Point>()
                && ImGui::MenuItem("Remove Point Light")) {

                Entity::remove_pbr_point_light(m_selected_entity);
            }
            if (m_selected_entity.has_component<Renderer::Light::Pbr::PointShadow>()
                && ImGui::MenuItem("Remove Point Light Shadow")) {

                Entity::remove_pbr_point_light_shadow(m_selected_entity);
            }
            if (m_selected_entity.has_component<Renderer::Light::Pbr::Spot>()
                && ImGui::MenuItem("Remove Spot Light")) {

                Entity::remove_pbr_spot_light(m_selected_entity);
            }
            if (m_selected_entity.has_component<Renderer::Light::Pbr::SpotShadow>()
                && ImGui::MenuItem("Remove Spot Light Shadow")) {

                Entity::remove_pbr_spot_light_shadow(m_selected_entity);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }
}

void EntitySelector::draw_model_prompt_window()
{
    if (!m_model_prompt.valid) {
        return;
    }

    ImGui::Begin("Add Model");

    ImGui::InputText("##ModelPathInput", m_model_prompt.path.data(), m_model_prompt.path.capacity());

    if (ImGui::Button("Add")) {
        Entity::add_mesh(m_model_prompt.entity, m_model_prompt.path.c_str());
        m_model_prompt.valid = false;
    };

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        m_model_prompt.valid = false;
    };

    ImGui::End();
}

void EntitySelector::draw_box_hull_prompt_window()
{
    if (!m_box_hull_prompt.valid) {
        return;
    }

    ImGui::Begin("Add Box Hull");

    ImGui::DragFloat3("Center", &m_box_hull_prompt.info.center.x, -20.0F, 20.0F);
    ImGui::DragFloat3("Extent", &m_box_hull_prompt.info.extent.x, -20.0F, 20.0F);

    ImGui::Checkbox("Preview", &m_box_hull_prompt.preview);

    if (m_box_hull_prompt.preview) {
        Utils::AABB aabb;
        aabb.min = m_box_hull_prompt.info.center - m_box_hull_prompt.info.extent;
        aabb.max = m_box_hull_prompt.info.center + m_box_hull_prompt.info.extent;
        auto mesh_transform = m_box_hull_prompt.entity.get_component<Utils::Transform>();

        Utils::Transform transform;
        transform.set_position(mesh_transform.get_position());
        transform.set_rotation(mesh_transform.get_rotation());

        aabb = aabb.transform(transform.get_model_matrix());

        g_app_data->m_line_renderer->add_aabb(aabb, Utils::Color::pack(Utils::Color::Green));
    }

    if (ImGui::Button("Add")) {
        Entity::add_box_hull_body(m_box_hull_prompt.entity, m_box_hull_prompt.info);
        m_box_hull_prompt.valid = false;
    };

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        m_box_hull_prompt.valid = false;
    };

    ImGui::End();
}
