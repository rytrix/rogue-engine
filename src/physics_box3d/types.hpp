#pragma once

namespace PhysicsBox3d {

enum class Type {
    Mesh,
    Shape,
    ConvexHull,
    BoxHull,
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

} // namespace PhysicsBox3d
