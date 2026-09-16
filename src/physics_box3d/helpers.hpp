#pragma once

#include "types.hpp"

namespace PhysicsBox3d {

constexpr glm::vec3 vec3_to_vec3(const b3Vec3& vec3)
{
    return { vec3.x, vec3.y, vec3.z };
}

constexpr b3Vec3 vec3_to_vec3(const glm::vec3& vec3)
{
    return { vec3.x, vec3.y, vec3.z };
}

constexpr glm::quat quat_to_quat(const b3Quat& quat)
{
    return { quat.s, quat.v.x, quat.v.y, quat.v.z };
}

constexpr b3Quat quat_to_quat(const glm::quat& quat)
{
    return { { quat.x, quat.y, quat.z }, quat.w };
}

constexpr b3BodyType body_type(MotionType motion)
{
    switch (motion) {
        case MotionType::Static:
            return b3_staticBody;
        case MotionType::Dynamic:
            return b3_dynamicBody;
        case MotionType::Kinematic:
            return b3_kinematicBody;
        default:
            util_error("Invalid MotionType");
    }
}

constexpr MotionType motion_type(b3BodyType motion)
{
    switch (motion) {
        case b3_staticBody:
            return MotionType::Static;
        case b3_dynamicBody:
            return MotionType::Dynamic;
        case b3_kinematicBody:
            return MotionType::Kinematic;
        default:
            util_error("Invalid BodyType");
    }
}

} // namespace PhysicsBox3d
