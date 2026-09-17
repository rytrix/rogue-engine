#include "aabb.hpp"

namespace Utils {

void AABB::update_points(const glm::vec3& point)
{
    min = glm::min(min, point);
    max = glm::max(max, point);
}

[[nodiscard]] AABB AABB::transform(const glm::mat4& transform) const
{
    return transform_fast(transform);
}

[[nodiscard]] bool AABB::intersection(const glm::vec3& position) const
{
    return position.x >= min.x && position.y >= min.y && position.z >= min.z
        && position.x <= max.x && position.y <= max.y && position.z <= max.z;
}

[[nodiscard]] AABB AABB::transform_naive(const glm::mat4& transform)
{
    AABB result;

    std::array<glm::vec3, 8> corners;
    corners[0] = transform * glm::vec4 { min, 1.0F };
    corners[1] = transform * glm::vec4 { max.x, min.y, min.z, 1.0F };
    corners[2] = transform * glm::vec4 { min.x, max.y, min.z, 1.0F };
    corners[3] = transform * glm::vec4 { min.x, min.y, max.z, 1.0F };
    corners[4] = transform * glm::vec4 { max.x, max.y, min.z, 1.0F };
    corners[5] = transform * glm::vec4 { max.x, max.y, max.z, 1.0F };
    corners[6] = transform * glm::vec4 { max.x, min.y, max.z, 1.0F };
    corners[7] = transform * glm::vec4 { min.x, max.y, max.z, 1.0F };

    result.min = corners[0];
    result.max = corners[0];
    for (u32 i = 1; i < corners.size(); i++) {
        const auto& corner = corners.at(i);
        result.min = glm::min(corner, result.min);
        result.max = glm::max(corner, result.max);
    }

    return result;
}

[[nodiscard]] AABB AABB::transform_fast(const glm::mat4& transform) const
{
    AABB result;

    // Start with the translation column of the matrix (Column 3)
    glm::vec3 translation = glm::vec3(transform[3]);
    result.min = translation;
    result.max = translation;

    // Loop through columns 0, 1, 2 (X, Y, Z basis vectors)
    for (int col = 0; col < 3; ++col) {
        for (int row = 0; row < 3; ++row) {
            float a = transform[col][row] * min[col];
            float b = transform[col][row] * max[col];

            if (a < b) {
                result.min[row] += a;
                result.max[row] += b;
            } else {
                result.min[row] += b;
                result.max[row] += a;
            }
        }
    }

    return result;
}

} // namespace Utils
