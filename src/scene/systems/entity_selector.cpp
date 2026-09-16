#include "entity_selector.hpp"

#define ENTITY_IMPL
#include "../entity.hpp"

#include "../../app_data.hpp"

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
                m_model_prompt.valid = false;
                event.m_consumed = true;
            }
        }
    }
}

void EntitySelector::update()
{
    if (!m_selected_entity.valid() && !g_global_app_data->m_capture_mouse) {
        auto ray_result = m_scene->m_physics_engine->ray_cast(Utils::ray_from_mouse(), g_global_app_data->m_camera->get_far());
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

    if (m_selected_entity.valid() && !g_global_app_data->m_capture_mouse) {
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

        if (selected_entity.has_component<PhysicsBox3d::EntityInfo>()) {
            auto& physics_info = selected_entity.get_component<PhysicsBox3d::EntityInfo>();
            if (physics_info.m_motion_type != PhysicsBox3d::MotionType::Static) {
                // check if this is still right...
                glm::vec3 pos = m_scene->m_physics_engine->get_body_pos(physics_info.m_id);
                glm::quat rot = m_scene->m_physics_engine->get_body_rot(physics_info.m_id);
                transform->set_position(pos);
                transform->set_rotation(rot);
            }
        }

        g_global_app_data->m_gizmo->m_transform = transform;
        g_global_app_data->m_gizmo->update();

        if (selected_entity.has_component<PhysicsBox3d::EntityInfo>()) {
            auto& physics_info = selected_entity.get_component<PhysicsBox3d::EntityInfo>();

            if (physics_info.m_motion_type != PhysicsBox3d::MotionType::Static) {
                glm::vec3 pos = m_scene->m_physics_engine->get_body_pos(physics_info.m_id);
                glm::quat rot = m_scene->m_physics_engine->get_body_rot(physics_info.m_id);
                m_scene->m_physics_engine->set_body_transform(physics_info.m_id, pos, rot);
            }
        }
        g_global_app_data->m_gizmo->draw();
    }

    draw_selected_entity_imgui();
    draw_model_prompt_window();
}

void EntitySelector::select_entity(Entity entity)
{
    util_assert(entity.valid(), "Trying to select an invalid entity");
    m_selected_entity = entity;
    g_global_app_data->m_gizmo->m_state = Gizmo::State::Translation;

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

    EntityComponents components {};

    auto entity_id = m_selected_entity.get_id();
    auto& registry = m_selected_entity.get_registry();
    auto* scene = m_selected_entity.get_scene();
    auto* physics_engine = scene->m_physics_engine.get();

    components.entity = m_selected_entity;
    components.scene = scene;

    constexpr float MAX_TRANSFORM = 64.0F;
    constexpr float MIN_TRANSFORM = -64.0F;

    constexpr float MAX_ROTATION = 360.0F;
    constexpr float MIN_ROTATION = -360.0F;

    constexpr float MAX_COLOR = 3000.0F;
    constexpr float MIN_COLOR = 0.0F;

    Utils::String no_name("no_name");
    Utils::String* name_check = registry.try_get<Utils::String>(entity_id);
    components.name = name_check == nullptr ? &no_name : name_check;

    components.mesh = registry.try_get<Renderer::Mesh*>(entity_id);
    components.animation_data = registry.try_get<Renderer::AnimationData>(entity_id);
    components.transform = registry.try_get<Utils::Transform>(entity_id);

    components.physics_info = registry.try_get<PhysicsBox3d::EntityInfo>(entity_id);

    components.point = registry.try_get<Renderer::Light::Pbr::Point>(entity_id);
    components.directional = registry.try_get<Renderer::Light::Pbr::Directional>(entity_id);
    components.spot = registry.try_get<Renderer::Light::Pbr::Spot>(entity_id);

    components.point_shadow = registry.try_get<Renderer::Light::Pbr::PointShadow>(entity_id);
    components.directional_shadow = registry.try_get<Renderer::Light::Pbr::DirectionalShadow>(entity_id);
    components.spot_shadow = registry.try_get<Renderer::Light::Pbr::SpotShadow>(entity_id);

    ImGui::Begin("Selected Entity");
    // if (!m_imgui_first_time.contains(name)) {
    //     ImGui::SetNextWindowSize(ImVec2(800, 600));
    //     m_imgui_first_time.insert(name);
    // }
    if (m_imgui_first_time) {
        ImGui::SetNextWindowSize(ImVec2(800, 600));
        m_imgui_first_time = false;
    }

    if (ImGui::InputText("##EntityNameInput", components.name->data(), components.name->capacity())) {
        *components.name = Utils::String(components.name->c_str());
    }

    draw_add_remove_component_imgui(components);

    if (ImGui::Button("Deselect Entity")) {
        g_global_app_data->m_entity_selector->deselect_entity();
        goto imgui_end_label;
    }

    if (components.mesh != nullptr && components.animation_data != nullptr) {
        auto& mesh = *components.mesh;
        auto& animation_data = *components.animation_data;

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
            scene->m_mesh_instance_draw_cache_needs_update = true;
        }

        ImGui::Checkbox("Pause Animation", &animation_data.paused);

        float ticks_per_second = animations[current_animation].get_ticks_per_second();
        if (ImGui::DragFloat("Ticks per second", &ticks_per_second)) {
            animations[current_animation].set_ticks_per_second(ticks_per_second);
        }
    }

    if (components.physics_info != nullptr) {
        auto& physics_info = *components.physics_info;
        auto& body_id = physics_info.m_id;
        auto& motion_type = physics_info.m_motion_type;

        if (motion_type != PhysicsBox3d::MotionType::Static) {
            ImGui::Text("Physics");
            glm::vec3 pos = scene->m_physics_engine->get_body_pos(body_id);
            glm::quat rot = scene->m_physics_engine->get_body_rot(body_id);
            // glm::vec3 cube_pos = Physics::vec3_to_vec3(body_interface->GetPosition(body_id));
            if (ImGui::DragFloat3("XYZ", &pos.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM)) {
                scene->m_physics_engine->set_body_transform(body_id, pos, rot);
            }

            glm::vec3 euler_angles = glm::degrees(glm::eulerAngles(rot));
            if (ImGui::DragFloat3("Rotation: XYZ", &euler_angles.x, 1.0F, MIN_ROTATION, MAX_ROTATION)) {
                scene->m_physics_engine->set_body_transform(body_id, pos, glm::quat(glm::radians(euler_angles)));
            }

            ImGui::Checkbox("Show Debug Physics Body Wireframe", &components.physics_info->m_should_debug_draw);
        } else {
            ImGui::Text("Physics - Static Object");

            auto& transform = *components.transform;
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
                scene->m_physics_engine->remove_body(body_id);

                // TODO: do I want this to just be my entity function? mainly for safety..
                physics_info = scene->m_physics_engine->create_mesh_body(Entity(scene, entity_id));

                // // auto new_physics_info = physics_info.m_physics_fn(m_physics_system.get(), Entity(scene, entity));
                // physics_info.m_id = new_physics_info.m_id;
                // physics_info.m_motion_type = new_physics_info.m_motion_type;

                // scene->m_physics_needs_optimize = true;
            }
        }
    }

    if (components.transform != nullptr && components.physics_info == nullptr) {
        auto& transform = *components.transform;

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

    if (components.point != nullptr) {
        auto& point = *components.point;

        ImGui::Text("Point Light");
        ImGui::DragFloat3("XYZ", &point.position.x, 1.0F, MIN_TRANSFORM, MAX_TRANSFORM);
        ImGui::DragFloat3("RGB", &point.color.x, 10.0F, MIN_COLOR, MAX_COLOR);
    }

    if (components.directional != nullptr) {
        auto& directional = *components.directional;

        ImGui::Text("Directional Light");
        ImGui::DragFloat3("XYZ", &directional.direction.x, 1.0F, -1.0F, 1.0F);
        ImGui::DragFloat3("RGB", &directional.color.x, 10.0F, MIN_COLOR, MAX_COLOR);
    }

    if (components.spot != nullptr) {
        auto& spot = *components.spot;

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

void EntitySelector::draw_add_remove_component_imgui(EntityComponents& components)
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
        if (components.mesh == nullptr && ImGui::MenuItem("Add Model")) {
            m_model_prompt.valid = true;
            m_model_prompt.entity = m_selected_entity;
            Utils::String string;
            m_model_prompt.path = string;
        }
        if (components.mesh != nullptr && components.physics_info == nullptr && ImGui::MenuItem("Add Static Body")) {
            Entity::add_static_body(components.entity);
        }
        if (components.mesh != nullptr && components.physics_info == nullptr && ImGui::BeginMenu("Add Dynamic Body")) {
            // if (ImGui::MenuItem("Box Shape")) {
            //     JPH::BoxShapeSettings settings(JPH::Vec3(0.5, 0.5, 0.5));
            //     JPH::Ref<JPH::Shape> shape = settings.Create().Get();
            //     Entity::add_dynamic_body(components.entity, shape);
            // }
            if (ImGui::MenuItem("Convex Hull Shape")) {
                Entity::add_convex_hull_body(components.entity);
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Add Light")) {
            bool has_no_lights = components.directional == nullptr && components.point == nullptr && components.spot == nullptr;
            bool has_no_other_lights = components.point == nullptr && components.spot == nullptr;
            if (has_no_lights && ImGui::MenuItem("Add Directional Light")) {
                Renderer::Light::Pbr::Directional info {};
                Entity::add_pbr_directional_light(components.entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Directional Light Shadow")) {
                Entity::add_pbr_directional_light_shadow(components.entity);
            }

            has_no_other_lights = components.directional == nullptr && components.spot == nullptr;
            if (has_no_lights && ImGui::MenuItem("Add Point Light")) {
                Renderer::Light::Pbr::Point info {};
                Entity::add_pbr_point_light(components.entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Point Light Shadow")) {
                Entity::add_pbr_point_light_shadow(components.entity);
            }

            has_no_other_lights = components.directional == nullptr && components.point == nullptr;
            if (has_no_lights && ImGui::MenuItem("Add Spot Light")) {
                Renderer::Light::Pbr::Spot info {};
                Entity::add_pbr_spot_light(components.entity, info);
            }
            if (has_no_other_lights && ImGui::MenuItem("Add Spot Light Shadow")) {
                Entity::add_pbr_spot_light_shadow(components.entity);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    if (ImGui::BeginPopup("Remove Component Popup")) {
        // if (ImGui::MenuItem("Name")) {
        // }
        // if (components.transform != nullptr && ImGui::MenuItem("Remove Transform")) {
        //     components.entity.remove_component<Transform>();
        // }
        if (components.mesh != nullptr && ImGui::MenuItem("Remove Model")) {
            components.entity.remove_component<Renderer::Mesh*>();
            if (components.entity.has_component<Renderer::AnimationData>()) {
                components.entity.remove_component<Renderer::AnimationData>();
            }
            components.scene->m_mesh_instance_draw_cache_needs_update = true;
        }
        if (components.physics_info != nullptr && components.physics_info->m_type == PhysicsBox3d::Type::Mesh && ImGui::MenuItem("Remove Static Body")) {
            components.scene->m_physics_engine->remove_body(components.physics_info->m_id);
            components.entity.remove_component<PhysicsBox3d::EntityInfo>();
            // components.scene->m_physics_needs_optimize = true;
        }
        if (components.physics_info != nullptr && components.physics_info->m_type != PhysicsBox3d::Type::Mesh && ImGui::MenuItem("Remove Dynamic Body")) {
            components.scene->m_physics_engine->remove_body(components.physics_info->m_id);
            components.entity.remove_component<PhysicsBox3d::EntityInfo>();
            // components.scene->m_physics_needs_optimize = true;
        }
        bool has_light = components.directional != nullptr || components.point != nullptr || components.spot != nullptr;
        if (has_light && ImGui::BeginMenu("Remove Light")) {
            if (components.directional != nullptr && ImGui::MenuItem("Remove Directional Light")) {
                components.entity.remove_component<Renderer::Light::Pbr::Directional>();
                if (components.directional_shadow != nullptr) {
                    components.entity.remove_component<Renderer::Light::Pbr::DirectionalShadow>();
                }
                components.scene->m_shaders_need_update = true;
            }
            if (components.directional_shadow != nullptr && ImGui::MenuItem("Remove Directional Light Shadow")) {
                components.entity.remove_component<Renderer::Light::Pbr::DirectionalShadow>();
                components.scene->m_shaders_need_update = true;
            }
            if (components.point != nullptr && ImGui::MenuItem("Remove Point Light")) {
                components.entity.remove_component<Renderer::Light::Pbr::Point>();
                if (components.point_shadow != nullptr) {
                    components.entity.remove_component<Renderer::Light::Pbr::PointShadow>();
                }
                components.scene->m_shaders_need_update = true;
            }
            if (components.point_shadow != nullptr && ImGui::MenuItem("Remove Point Light Shadow")) {
                components.entity.remove_component<Renderer::Light::Pbr::PointShadow>();
                components.scene->m_shaders_need_update = true;
            }
            if (components.spot != nullptr && ImGui::MenuItem("Remove Spot Light")) {
                components.entity.remove_component<Renderer::Light::Pbr::Spot>();
                if (components.spot_shadow != nullptr) {
                    components.entity.remove_component<Renderer::Light::Pbr::SpotShadow>();
                }
                components.scene->m_shaders_need_update = true;
            }
            if (components.spot_shadow != nullptr && ImGui::MenuItem("Remove Spot Light Shadow")) {
                components.entity.remove_component<Renderer::Light::Pbr::SpotShadow>();
                components.scene->m_shaders_need_update = true;
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
