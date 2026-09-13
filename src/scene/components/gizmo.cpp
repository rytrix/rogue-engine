#include "gizmo.hpp"

#include "../../app_data.hpp"

#include "../../utils/color.hpp"
#include "../../utils/math/line.hpp"

#include "../transform.hpp"

#include "../../physics_jolt/interface.hpp"

Gizmo::Gizmo(GlobalAppData* app_data)
    : m_app_data(app_data)
{
}

Gizmo::Gizmo(GlobalAppData* app_data, Transform* transform)
    : m_transform(transform)
    , m_app_data(app_data)
{
}

void Gizmo::init(GlobalAppData* app_data)
{
    m_app_data = app_data;
}

void Gizmo::init(GlobalAppData* app_data, Transform* transform)
{
    m_app_data = app_data;
    m_transform = transform;
}

void Gizmo::on_event(Event& event)
{
    if (m_transform == nullptr) {
        return;
    }

    if (event.m_consumed) {
        return;
    }

    if (event.m_type == Event::Type::SDL) {
        if (event.m_sdl_event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
            if (event.m_sdl_event.button.button == SDL_BUTTON_LEFT) {
                test_intersection();
#ifdef GIZMO_DEBUG_RAY
                m_prev_ray = Utils::ray_from_mouse(m_app_data);
#endif
            }
        } else if (event.m_sdl_event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
            if (event.m_sdl_event.button.button == SDL_BUTTON_LEFT) {
                m_prev_hit.on_down = false;
            }
        }
    }
}

void Gizmo::update()
{
    if (m_transform == nullptr) {
        return;
    }

    if (m_prev_hit.on_down == false) {
        return;
    }

    auto ray = Utils::ray_from_mouse(m_app_data);
    glm::vec3 position = m_transform->get_position();

    if (m_state == State::Rotation) {
        Utils::Plane plane {};
        plane.normal = m_prev_hit.normal;
        plane.position = position;

        auto result = Utils::intersect_ray_plane(ray, plane);
        if (result.has_value()) {
            auto hit = result.value();
            // Use trig to find the angle between that and the new hitpoint
            // Add the angle to the stored translation

            glm::vec3 a = glm::normalize(m_prev_hit.hit - position);
            glm::vec3 b = glm::normalize(hit - position);

            float dot = glm::dot(a, b);
            dot = glm::clamp(dot, -1.0F, 1.0F);

            float angle = glm::degrees(glm::acos(dot));

            // If the cross product is opposite the rings normal flip the sign
            glm::vec3 cross_prod = glm::cross(a, b);
            if (glm::dot(cross_prod, plane.normal) < 0.0F) {
                angle = -angle;
            }

            float angle_difference = angle - m_prev_hit.prev_rotation;
            m_prev_hit.prev_rotation = angle;

            m_transform->rotate(angle_difference, plane.normal);
        }
    } else {
        Utils::Plane plane {};
        plane.normal = m_prev_hit.normal;
        plane.position = position;

        auto result = Utils::intersect_ray_plane(ray, plane);
        if (result.has_value()) {
            auto hit = result.value();

            glm::vec3 vector_to_hit = hit - plane.position;
            float length_along_line = glm::dot(vector_to_hit, m_prev_hit.direction);
            glm::vec3 closest_point = position + (length_along_line * m_prev_hit.direction);

            glm::vec3 movement_vector = closest_point - m_prev_hit.hit;
            float distance = glm::dot(movement_vector, m_prev_hit.direction);

            if (m_state == State::Translation) {
                glm::vec3 new_position = m_prev_hit.position + m_prev_hit.direction * distance;
                m_transform->set_position(new_position);
            } else {
                glm::vec3 scale = m_prev_hit.scale + m_prev_hit.outward_direction * distance;
                m_transform->set_scale(scale);
            }
        }
    }
}

void Gizmo::draw()
{
    if (m_transform == nullptr) {
        return;
    }

    switch (m_state) {
        case State::Translation:
        case State::Scale:
            batch_lines(get_radius());
            break;
        case State::Rotation:
            batch_rotations(get_radius());
            break;
    }
#ifdef GIZMO_DEBUG_RAY
    if (m_prev_ray.has_value()) {
        m_app_data->line_renderer.add_ray(m_prev_ray.value(), 50.0F, Utils::Color::Red);
    }
#endif
    imgui_ui();
}

void Gizmo::test_intersection()
{
    if (m_prev_hit.on_down) {
        return;
    }

    if (m_state == State::Rotation) {
        test_intersection_rotation();
    } else {
        test_intersection_lines();
    }
}

void Gizmo::test_intersection_lines()
{
    auto ray = Utils::ray_from_mouse(m_app_data);
    auto view_direction = glm::normalize(m_transform->get_position() - ray.position);
    auto radius = get_radius();

    Utils::Line line {};
    line.length = radius * 2;
    line.thickness = LINE_THICKNESS + glm::distance(ray.position, m_transform->get_position()) / LINE_THICKNESS_DOUBLE_DISTANCE + LINE_THICKNESS;

    float closest_distance = std::numeric_limits<float>::max();

    auto handle_result = [&](Utils::RayLineResult result) {
        auto [hit, closest] = result;
        float distance = glm::distance(hit, closest);
        if (distance < closest_distance) {
            closest_distance = distance;

            m_prev_hit.on_down = true;
            m_prev_hit.hit = closest;
            m_prev_hit.direction = line.direction;
            m_prev_hit.scale = m_transform->get_scale();
            // closest_point - transform_position
            m_prev_hit.outward_direction = glm::normalize(closest - m_transform->get_position());
            m_prev_hit.position = m_transform->get_position();
        }
    };

    // Get a normal that directly faces the camera
    auto normal = view_direction;

    line.position = m_transform->get_position() - glm::vec3(0.0, radius, 0.0);
    line.direction = glm::vec3(0.0, 1.0, 0.0);

    auto result = Utils::intersect_ray_line_closest(ray, line);
    if (result.has_value()) {
        m_prev_hit.normal = glm::normalize(glm::vec3(normal.x, 0.0, normal.z));
        handle_result(result.value());
    }

    line.position = m_transform->get_position() - glm::vec3(radius, 0.0, 0.0);
    line.direction = glm::vec3(1.0, 0.0, 0.0);

    result = Utils::intersect_ray_line_closest(ray, line);
    if (result.has_value()) {
        m_prev_hit.normal = glm::normalize(glm::vec3(0.0, normal.y, normal.z));
        handle_result(result.value());
    }

    line.position = m_transform->get_position() - glm::vec3(0.0, 0.0, radius);
    line.direction = glm::vec3(0.0, 0.0, 1.0);

    result = Utils::intersect_ray_line_closest(ray, line);
    if (result.has_value()) {
        m_prev_hit.normal = glm::normalize(glm::vec3(normal.x, normal.y, 0.0));
        handle_result(result.value());
    }
}

void Gizmo::test_intersection_rotation()
{
    auto ray = Utils::ray_from_mouse(m_app_data);

    Utils::Ring ring {};
    ring.position = m_transform->get_position();
    ring.radius = get_radius();
    ring.thickness = LINE_THICKNESS + glm::distance(ray.position, ring.position) / LINE_THICKNESS_DOUBLE_DISTANCE * LINE_THICKNESS;

    float closest_distance = std::numeric_limits<float>::max();

    auto handle_result = [&](Utils::RayRingResult result) {
        auto [hit, distance] = result;
        if (distance < closest_distance) {
            closest_distance = distance;
            m_prev_hit.on_down = true;
            m_prev_hit.hit = hit;
            m_prev_hit.normal = ring.normal;
            m_prev_hit.prev_rotation = 0.0F;
        }
    };

    ring.normal = glm::vec3(1.0, 0.0, 0.0);
    auto result = Utils::intersect_ray_ring(ray, ring);
    if (result.has_value()) {
        handle_result(result.value());
    }

    ring.normal = glm::vec3(0.0, 1.0, 0.0);
    result = Utils::intersect_ray_ring(ray, ring);
    if (result.has_value()) {
        handle_result(result.value());
    }

    ring.normal = glm::vec3(0.0, 0.0, 1.0);
    result = Utils::intersect_ray_ring(ray, ring);
    if (result.has_value()) {
        handle_result(result.value());
    }
}

f32 Gizmo::get_radius()
{
    auto camera_pos = m_app_data->m_camera.get_pos();
    auto gizmo_pos = m_transform->get_position();
    return m_radius + glm::distance(camera_pos, gizmo_pos) / RADIUS_DOUBLE_DISTANCE * m_radius;
}

void Gizmo::batch_rotations(f32 radius)
{
    Transform transform;
    transform.set_position(m_transform->get_position());
    m_app_data->m_line_renderer.add_circle(transform.get_model_matrix(), radius, Utils::Color::Blue);
    transform.set_euler_angles(glm::vec3(90.0, 0.0, 0.0));
    m_app_data->m_line_renderer.add_circle(transform.get_model_matrix(), radius, Utils::Color::Green);
    transform.set_euler_angles(glm::vec3(0.0, 90.0, 0.0));
    m_app_data->m_line_renderer.add_circle(transform.get_model_matrix(), radius, Utils::Color::Red);
}

void Gizmo::batch_lines(f32 radius)
{
    Transform transform;
    transform.set_position(m_transform->get_position());
    m_app_data->m_line_renderer.add_line(transform.get_model_matrix(),
        glm::vec3(0.0, -radius, 0.0), glm::vec3(0.0, radius, 0.0), Utils::Color::Blue);
    m_app_data->m_line_renderer.add_line(transform.get_model_matrix(),
        glm::vec3(-radius, 0.0, 0.0), glm::vec3(radius, 0.0, 0.0), Utils::Color::Green);
    m_app_data->m_line_renderer.add_line(transform.get_model_matrix(),
        glm::vec3(0.0, 0.0, -radius), glm::vec3(0.0, 0.0, radius), Utils::Color::Red);
}

void Gizmo::imgui_ui()
{
    ImGui::Begin("Gizmo");

    if (m_app_data->m_entity_selector.m_selected_entity.valid()) {
        if (ImGui::Button("Gizmo Translation")) {
            m_app_data->m_gizmo.m_state = Gizmo::State::Translation;
        }
        ImGui::SameLine();
        if (ImGui::Button("Gizmo Rotation")) {
            m_app_data->m_gizmo.m_state = Gizmo::State::Rotation;
        }
        if (!m_app_data->m_entity_selector.m_selected_entity.has_component<Physics::PhysicsInfo>()
            || (m_app_data->m_entity_selector.m_selected_entity.has_component<Physics::PhysicsInfo>()
                && m_app_data->m_entity_selector.m_selected_entity.get_component<Physics::PhysicsInfo>().m_motion_type == JPH::EMotionType::Static)) {
            ImGui::SameLine();
            if (ImGui::Button("Gizmo Scale")) {
                m_app_data->m_gizmo.m_state = Gizmo::State::Scale;
            }
        }
        if (ImGui::Button("Deselect Entity")) {
            m_app_data->m_entity_selector.deselect_entity();
        }
    }

    ImGui::End();
}
