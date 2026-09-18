#pragma once

#include "system.hpp"
#include "../entity.hpp"
#include "../event.hpp"
#include "../../utils/string.hpp"
#include "../scene.hpp"

class EntitySelector : public NoCopyNoMove, public System {
public:
    EntitySelector(Scene* scene);

    void init(Scene* scene);

    void on_event(Event& event) override;
    void update() override;
    void draw() override;

    void select_entity(Entity entity);
    void deselect_entity();

    Entity m_hovered_entity;
    Entity m_selected_entity;

    bool m_hover_enabled = true;

private:
    Scene* m_scene = nullptr;

    enum State {
        On,
        Off,
        Invalid
    };
    State m_prev_physics_state = State::Off;

    struct AddModelPrompt {
        bool valid = false;
        Entity entity;
        Utils::String path;
    };
    AddModelPrompt m_model_prompt {};

    struct AddBoxHullPrompt {
        bool valid = false;
        Entity entity;
        PhysicsBox3d::BoxHullInfo info;
        bool preview = false;
    };
    AddBoxHullPrompt m_box_hull_prompt {};

    struct AddCapsulePrompt {
        bool valid = false;
        Entity entity;
        PhysicsBox3d::CapsuleInfo info;
        bool preview = false;
    };
    AddCapsulePrompt m_capsule_prompt {};

    bool m_imgui_first_time = true;
    void draw_selected_entity_imgui();
    void draw_add_remove_component_imgui();

    void draw_model_prompt_window();
    void draw_box_hull_prompt_window();
    void draw_capsule_prompt_window();
};
