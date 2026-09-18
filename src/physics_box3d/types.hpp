#pragma once

namespace PhysicsBox3d {

enum class Type {
    Mesh,
    Shape,
    ConvexHull,
    BoxHull,
    Capsule,
};

enum class MotionType {
    Static,
    Kinematic,
    Dynamic,
};

struct BoxHullInfo {
    glm::vec3 center;
    glm::vec3 extent;
};

struct CapsuleInfo {
    glm::vec3 center_bottom;
    glm::vec3 center_top;
    f32 radius;
};

} // namespace PhysicsBox3d
