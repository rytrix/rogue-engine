#include "interface.hpp"
#include "Jolt/Geometry/ConvexHullBuilder.h"
#include "Jolt/Physics/Collision/Shape/ConvexHullShape.h"
#include "helpers.hpp"

#include "engine.hpp"

namespace Physics {

PhysicsInfo create_static_body(Entity entity)
{
    PhysicsInfo info {};

    JPH::TriangleList triangles;
    const auto* mesh = entity.get_component<Renderer::Mesh*>();
    if (entity.has_component<Transform>()) {
        auto& transform = entity.get_component<Transform>();
        Physics::System::create_mesh_triangle_list_base_index(triangles, transform.get_model_matrix(), mesh);
    } else {
        Physics::System::create_mesh_triangle_list_base_index(triangles, mesh);
    }

    JPH::MeshShapeSettings mesh_settings(triangles);
    mesh_settings.Sanitize();
    JPH::BodyCreationSettings body_settings(mesh_settings.Create().Get(),
        JPH::RVec3::sZero(), JPH::Quat::sIdentity(),
        JPH::EMotionType::Static,
        Physics::Layers::NON_MOVING);

    auto* system = entity.get_scene()->m_physics_system.get();

    JPH::BodyID body_id
        = system->m_body_interface->CreateAndAddBody(
            body_settings,
            JPH::EActivation::DontActivate);

    info.m_id = body_id;
    info.m_motion_type = JPH::EMotionType::Static;

    info.m_type = PhysicsType::Mesh;
    info.m_entity = entity;

    return info;
}

PhysicsInfo create_dynamic_body(Entity entity, JPH::Ref<JPH::Shape> shape)
{
    PhysicsInfo info {};

    JPH::RVec3 position { 0.0, 0.0, 0.0 };
    JPH::Quat rotation { JPH::Quat::sIdentity() };

    if (entity.has_component<Transform>()) {
        auto& transform = entity.get_component<Transform>();

        position = Physics::vec3_to_vec3(transform.get_position());
        rotation = Physics::quat_to_quat(transform.get_rotation());
    }

    JPH::BodyCreationSettings settings(
        shape,
        position,
        rotation,
        JPH::EMotionType::Dynamic,
        Physics::Layers::MOVING);

    auto* system = entity.get_scene()->m_physics_system.get();

    auto body = system->m_body_interface->CreateAndAddBody(
        settings,
        JPH::EActivation::Activate);

    info.m_entity = entity;
    info.m_shape = shape;

    info.m_id = body;
    info.m_motion_type = JPH::EMotionType::Dynamic;
    info.m_type = PhysicsType::Shape;

    return info;
}

PhysicsInfo create_convex_hull(Entity entity)
{
    PhysicsInfo info {};

    JPH::RVec3 position { 0.0, 0.0, 0.0 };
    JPH::Quat rotation { JPH::Quat::sIdentity() };

    JPH::Array<JPH::Vec3> triangles;
    const auto* mesh = entity.get_component<Renderer::Mesh*>();
    if (entity.has_component<Transform>()) {
        auto& transform = entity.get_component<Transform>();

        position = Physics::vec3_to_vec3(transform.get_position());
        rotation = Physics::quat_to_quat(transform.get_rotation());

        transform.set_position(glm::vec3(0.0, 0.0, 0.0));
        transform.set_rotation(glm::vec3(0.0, 0.0, 0.0));

        Physics::System::create_mesh_vec3s_base_index(triangles, transform.get_model_matrix(), mesh);
    } else {
        Physics::System::create_mesh_vec3s_base_index(triangles, mesh);
    }

    JPH::ConvexHullShapeSettings convex_hull_settings(triangles);
    info.m_shape = convex_hull_settings.Create().Get();

    JPH::BodyCreationSettings body_settings(info.m_shape,
        position, rotation,
        JPH::EMotionType::Dynamic,
        Physics::Layers::MOVING);

    body_settings.mMotionQuality = JPH::EMotionQuality::LinearCast;

    auto* system = entity.get_scene()->m_physics_system.get();

    auto body = system->m_body_interface->CreateAndAddBody(
        body_settings,
        JPH::EActivation::Activate);

    info.m_entity = entity;

    info.m_id = body;
    info.m_motion_type = JPH::EMotionType::Dynamic;
    info.m_type = PhysicsType::ConvexHull;

    return info;
}

} // namespace Physics
