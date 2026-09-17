#pragma once

namespace Utils {

struct AABB {
    glm::vec3 min { std::numeric_limits<float>::max() };
    glm::vec3 max { -std::numeric_limits<float>::max() };

    // Helper function for model loader
    // Replaces min and max coordinates with
    // new min/max values
    void update_points(const glm::vec3& point);

    [[nodiscard]] AABB transform(const glm::mat4& transform) const;
    [[nodiscard]] bool intersection(const glm::vec3& position) const;

private:
    [[nodiscard]] AABB transform_naive(const glm::mat4& transform);
    [[nodiscard]] AABB transform_fast(const glm::mat4& transform) const;
};

} // namespace Utils
