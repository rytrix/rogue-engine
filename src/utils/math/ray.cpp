#include "ray.hpp"

#include "../../app_data.hpp"

namespace Utils {

static constexpr f32 PARALLEL_PRECISION = 1e-6F;

#define PARALLEL_TO_PLANE_CHECK

Ray::Ray(glm::vec3 in_position, glm::vec3 in_direction)
    : position(in_position)
    , direction(in_direction)
{
}

glm::vec3 Ray::get_inverse()
{
    if (inverse_needs_update) {
        inv_direction = 1.0F / direction;
        inverse_needs_update = false;
    }
    return inv_direction;
}

[[nodiscard]] Utils::Ray ray_from_mouse()
{
    glm::mat4 inv_proj_view = g_global_app_data->m_camera->get_inverse_proj_view();

    glm::vec2 mouse_pos = g_global_app_data->m_window->get_mouse_pos();
    glm::vec2 screen_size = g_global_app_data->m_window->get_size_f32();

    glm::vec2 screen_coord = mouse_pos / screen_size;
    screen_coord.y = 1.0F - screen_coord.y;
    screen_coord = 2.0F * screen_coord - 1.0F;

    glm::vec4 target
        = inv_proj_view * glm::vec4(screen_coord.x, screen_coord.y, 1.0F, 1.0F);

    glm::vec3 world_point = glm::vec3(target) / target.w;
    glm::vec3 camera_pos = g_global_app_data->m_camera->get_pos();
    glm::vec3 ray_dir = glm::normalize(world_point - camera_pos);

    return { g_global_app_data->m_camera->get_pos(), ray_dir };
}

[[nodiscard]] Utils::Ray ray_from_center()
{
    glm::mat4 inv_proj_view = g_global_app_data->m_camera->get_inverse_proj_view();
    glm::vec4 target
        = inv_proj_view * glm::vec4(0.0F, 0.0F, 1.0F, 1.0F);

    glm::vec3 world_point = glm::vec3(target) / target.w;
    glm::vec3 camera_pos = g_global_app_data->m_camera->get_pos();
    glm::vec3 ray_dir = glm::normalize(world_point - camera_pos);

    return { g_global_app_data->m_camera->get_pos(), ray_dir };
}

[[nodiscard]] std::optional<RayRingResult> intersect_ray_ring(const Ray& ray, const Ring& ring)
{
    float denom = glm::dot(ray.direction, ring.normal);

#ifdef PARALLEL_TO_PLANE_CHECK
    if (glm::abs(denom) < PARALLEL_PRECISION) {
        return std::nullopt; // Parallel to plane
    }
#endif

    float t = glm::dot(ring.position - ray.position, ring.normal) / denom;

    if (t < 0.0F) {
        return std::nullopt;
    }

    glm::vec3 hit_point = ray.position + (ray.direction * t);

    float dist_from_center = glm::distance(hit_point, ring.position);

    float dist_from_radius = glm::abs(dist_from_center - ring.radius);
    if (dist_from_radius <= ring.thickness * 0.5) {
        return RayRingResult { .hit = hit_point, .distance = dist_from_radius };
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<glm::vec3> intersect_ray_plane(const Ray& ray, const Plane& plane)
{
    float denom = glm::dot(ray.direction, plane.normal);

#ifdef PARALLEL_TO_PLANE_CHECK
    if (glm::abs(denom) < PARALLEL_PRECISION) {
        return std::nullopt; // Parallel to plane
    }
#endif

    float t = glm::dot(plane.position - ray.position, plane.normal) / denom;

    if (t < 0.0F) {
        return std::nullopt;
    }

    glm::vec3 hit_point = ray.position + (ray.direction * t);

    return hit_point;
}

[[nodiscard]] std::optional<glm::vec3> intersect_ray_line(const Ray& ray, const Line& line)
{
    glm::vec3 ray_dir = ray.direction;
    glm::vec3 line_dir = line.direction;
    glm::vec3 origin_offset = ray.position - line.position;

    float dot_directions = glm::dot(ray_dir, line_dir);
    float dot_ray_offset = glm::dot(ray_dir, origin_offset);
    float dot_line_offset = glm::dot(line_dir, origin_offset);

    float denominator = 1.0F - (dot_directions * dot_directions);
    float dist_along_ray = 0.0F;
    float dist_along_line = 0.0F;

    // Handle parallel lines to avoid division by zero
    if (denominator < PARALLEL_PRECISION) {
        dist_along_ray = 0.0F;
        dist_along_line = dot_line_offset;
    } else {
        dist_along_ray = (dot_directions * dot_line_offset - 1.0F * dot_ray_offset) / denominator;
        dist_along_line = (1.0F * dot_line_offset - dot_directions * dot_ray_offset) / denominator;
    }

    // The ray is pointing away from the line
    if (dist_along_ray < 0.0F) {
        return std::nullopt;
    }

    glm::vec3 ray_hit_point = ray.position + (dist_along_ray * ray_dir);
    glm::vec3 closest_point = line.position + (dist_along_line * line_dir);

    bool within_bounds = (dist_along_line >= 0.0F) && (dist_along_line <= line.length);

    float distance_to_line = glm::distance(ray_hit_point, closest_point);
    bool is_touching_line = distance_to_line <= line.thickness;

    if (within_bounds && is_touching_line) {
        return ray_hit_point;
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<RayLineResult> intersect_ray_line_closest(const Ray& ray, const Line& line)
{
    glm::vec3 ray_dir = ray.direction;
    glm::vec3 line_dir = line.direction;
    glm::vec3 origin_offset = ray.position - line.position;

    float dot_directions = glm::dot(ray_dir, line_dir);
    float dot_ray_offset = glm::dot(ray_dir, origin_offset);
    float dot_line_offset = glm::dot(line_dir, origin_offset);

    float denominator = 1.0F - (dot_directions * dot_directions);
    float dist_along_ray = 0.0F;
    float dist_along_line = 0.0F;

    // Handle parallel lines to avoid division by zero
    if (denominator < PARALLEL_PRECISION) {
        dist_along_ray = 0.0F;
        dist_along_line = dot_line_offset;
    } else {
        dist_along_ray = (dot_directions * dot_line_offset - 1.0F * dot_ray_offset) / denominator;
        dist_along_line = (1.0F * dot_line_offset - dot_directions * dot_ray_offset) / denominator;
    }

    // The ray is pointing away from the line
    if (dist_along_ray < 0.0F) {
        return std::nullopt;
    }

    glm::vec3 ray_hit_point = ray.position + (dist_along_ray * ray_dir);
    glm::vec3 closest_point = line.position + (dist_along_line * line_dir);

    bool within_bounds = (dist_along_line >= 0.0F) && (dist_along_line <= line.length);

    float distance_to_line = glm::distance(ray_hit_point, closest_point);
    bool is_touching_line = distance_to_line <= line.thickness;

    if (within_bounds && is_touching_line) {
        return RayLineResult { .hit = ray_hit_point, .closest = closest_point };
    }

    return std::nullopt;
}

[[nodiscard]] bool intersect_ray_aabb(Ray& ray, const AABB& aabb)
{
    glm::vec3 t0 = (aabb.min - ray.position) * ray.get_inverse();
    glm::vec3 t1 = (aabb.max - ray.position) * ray.get_inverse();

    glm::vec3 t_near = glm::min(t0, t1);
    glm::vec3 t_far = glm::max(t0, t1);

    float t_min = std::max({ t_near.x, t_near.y, t_near.z });
    float t_max = std::min({ t_far.x, t_far.y, t_far.z });

    return t_max >= std::max(0.0f, t_min);
}

[[nodiscard]] std::optional<RayAABBResult> intersect_ray_aabb_hit(Ray& ray, const AABB& aabb)
{
    glm::vec3 t0 = (aabb.min - ray.position) * ray.get_inverse();
    glm::vec3 t1 = (aabb.max - ray.position) * ray.get_inverse();

    glm::vec3 t_near = glm::min(t0, t1);
    glm::vec3 t_far = glm::max(t0, t1);

    float t_min = std::max({ t_near.x, t_near.y, t_near.z });
    float t_max = std::min({ t_far.x, t_far.y, t_far.z });

    if (t_max >= std::max(0.0f, t_min)) {
        float t_hit = std::max(0.0f, t_min);
        glm::vec3 hit = ray.position + (t_hit * ray.direction);
        return RayAABBResult { .hit = hit, .distance = t_hit };
    }

    return std::nullopt;
}

} // namespace Utils
