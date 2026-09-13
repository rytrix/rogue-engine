#pragma once

namespace Physics {

static constexpr JPH::Float3 vec3_to_float3(glm::vec3 vec)
{
    return { vec.x, vec.y, vec.z };
}

static constexpr glm::vec3 vec3_to_vec3(JPH::Vec3 vec)
{
    return { vec.GetX(), vec.GetY(), vec.GetZ() };
}

static constexpr JPH::Vec3 vec3_to_vec3(glm::vec3 vec)
{
    return { vec.x, vec.y, vec.z };
}

static constexpr JPH::DVec3 vec3_to_dvec3(glm::vec3 vec)
{
    return { static_cast<double>(vec.x), static_cast<double>(vec.y), static_cast<double>(vec.z) };
}

static constexpr JPH::Quat quat_to_quat(glm::quat quat)
{
    return { quat.x, quat.y, quat.z, quat.w };
}

static constexpr glm::quat quat_to_quat(JPH::Quat quat)
{
    return { quat.GetW(), quat.GetX(), quat.GetY(), quat.GetZ() };
}

static constexpr glm::mat4 mat4_to_mat4(JPH::Mat44 mat4)
{
    glm::mat4 out;
    for (u32 row = 0; row < 4; row++) {
        for (u32 col = 0; col < 4; col++) {
            out[static_cast<int>(col)][static_cast<int>(row)] = mat4(row, col);
        }
    }

    return out;
}

static constexpr glm::mat4 mat4_to_mat4(JPH::DMat44 dmat4)
{
    JPH::Mat44 mat4 = dmat4.ToMat44();
    glm::mat4 out;
    for (u32 row = 0; row < 4; row++) {
        for (u32 col = 0; col < 4; col++) {
            out[static_cast<int>(col)][static_cast<int>(row)] = mat4(row, col);
        }
    }

    return out;
}

} // namespace Physics
