#pragma once

struct GlobalAppData;

#include "system.hpp"
#include "../../utils/transform.hpp"

#include "../event.hpp"

class Gizmo : public NoCopyNoMove, public System {
public:
    enum class State {
        Translation,
        Rotation,
        Scale,
    };
    Gizmo() = default;

    Gizmo(Utils::Transform* transform);

    void init();
    void init(Utils::Transform* transform);

    void on_event(Event& event) override;
    void update() override;

    // This function only batches to the Line Renderer
    // app_data->debug_renderer.draw() has to be called after
    void draw() override;

    State m_state = State::Translation;
    f32 m_radius = 2.0;

    Utils::Transform* m_transform = nullptr;

private:
    static constexpr f32 LINE_THICKNESS = 0.3F;
    static constexpr f32 LINE_THICKNESS_DOUBLE_DISTANCE = 50.0F;
    static constexpr f32 RADIUS_DOUBLE_DISTANCE = 10.0F;

    struct PreviousHit {
        bool on_down;
        glm::vec3 hit;
        glm::vec3 normal;
        glm::vec3 direction;
        glm::vec3 outward_direction;

        glm::vec3 position;
        glm::vec3 scale;
        float prev_rotation;
    };

#ifdef GIZMO_DEBUG_RAY
    std::optional<Utils::Ray> m_prev_ray;
#endif

    PreviousHit m_prev_hit {};

    void test_intersection();
    void test_intersection_lines();
    void test_intersection_rotation();

    // Get a radius value that scales up with distance
    f32 get_radius();

    void batch_rotations(f32 radius);
    void batch_lines(f32 radius);
    
    void imgui_ui();
};
