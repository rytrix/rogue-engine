#pragma once

#include "aabb.hpp"
#include "line.hpp"
#include "plane.hpp"
#include "ring.hpp"

struct GlobalAppData;

namespace Utils {

struct Ray {
    glm::vec3 position;
    glm::vec3 direction;

    Ray(glm::vec3 in_position, glm::vec3 in_direction);

    glm::vec3 get_inverse();

private:
    bool inverse_needs_update = true;
    glm::vec3 inv_direction {};
};

[[nodiscard]] Utils::Ray ray_from_mouse();
[[nodiscard]] Utils::Ray ray_from_center();

struct RayRingResult {
    glm::vec3 hit;
    float distance;
};
[[nodiscard]] std::optional<RayRingResult> intersect_ray_ring(const Ray& ray, const Ring& ring);

[[nodiscard]] std::optional<glm::vec3> intersect_ray_plane(const Ray& ray, const Plane& plane);

struct RayLineResult {
    glm::vec3 hit;
    glm::vec3 closest;
};
[[nodiscard]] std::optional<glm::vec3> intersect_ray_line(const Ray& ray, const Line& line);
[[nodiscard]] std::optional<RayLineResult> intersect_ray_line_closest(const Ray& ray, const Line& line);

struct RayAABBResult {
    glm::vec3 hit;
    float distance;
};
[[nodiscard]] bool intersect_ray_aabb(Ray& ray, const AABB& aabb);
[[nodiscard]] std::optional<RayAABBResult> intersect_ray_aabb_hit(Ray& ray, const AABB& aabb);

} // namespace Utils
