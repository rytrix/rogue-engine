#pragma once

#include "../scene/entity.hpp"

namespace Physics {

// Documentation
// https://jrouwe.github.io/JoltPhysicsDocs/3.0.1/

enum class PhysicsType {
    Mesh,
    Shape,
    ConvexHull
};

struct PhysicsInfo {
    JPH::BodyID m_id;
    JPH::EMotionType m_motion_type {};

    PhysicsType m_type;

    bool m_should_debug_draw = false;

    JPH::Ref<JPH::Shape> m_shape;
    Entity m_entity;
};

PhysicsInfo create_static_body(Entity entity);
PhysicsInfo create_dynamic_body(Entity entity, JPH::Ref<JPH::Shape> shape);
PhysicsInfo create_convex_hull(Entity entity);

} // namespace Physics
