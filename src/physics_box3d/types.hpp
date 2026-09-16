#pragma once

namespace PhysicsBox3d {

enum class Type {
    Mesh,
    Shape,
    ConvexHull
};

enum class MotionType {
    Static,
    Kinematic,
    Dynamic,
};

} // namespace PhysicsBox3d
