#pragma once

#include "../entity.hpp"
#include "../event.hpp"
#include "../../utils/string.hpp"
#include "../../renderer/mesh.hpp"
#include "../scene.hpp"

class EntitySelector : public NoCopyNoMove {
public:
    EntitySelector(Scene* scene);

    void init(Scene* scene);

    void on_event(Event& event);
    void update();
    void draw();

    void select_entity(Entity entity);
    void deselect_entity();

    Entity m_hovered_entity;
    Entity m_selected_entity;

private:
    Scene* m_scene = nullptr;

    enum State {
        On,
        Off,
        Invalid
    };
    State m_prev_physics_state = State::Off;

    struct EntityComponents {
        Entity entity;
        Scene* scene = nullptr;
        Utils::String* name = nullptr;
        Renderer::Mesh** mesh = nullptr;
        Renderer::AnimationData* animation_data = nullptr;
        Utils::Transform* transform = nullptr;
        PhysicsBox3d::EntityInfo* physics_info = nullptr;
        Renderer::Light::Pbr::Point* point = nullptr;
        Renderer::Light::Pbr::PointShadow* point_shadow = nullptr;
        Renderer::Light::Pbr::Directional* directional = nullptr;
        Renderer::Light::Pbr::DirectionalShadow* directional_shadow = nullptr;
        Renderer::Light::Pbr::Spot* spot = nullptr;
        Renderer::Light::Pbr::SpotShadow* spot_shadow = nullptr;
    };

    struct AddModelPrompt {
        bool valid = false;
        Entity entity;
        Utils::String path;
    };
    AddModelPrompt m_model_prompt {};

    bool m_imgui_first_time = true;
    void draw_selected_entity_imgui();
    void draw_add_remove_component_imgui(EntityComponents& components);
    void draw_model_prompt_window();
};
