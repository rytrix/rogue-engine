#include "line_renderer.hpp"

#include "../renderer/shader_preprocessor.hpp"

#include "../utils/color.hpp"
#include "../utils/file.hpp"

namespace Renderer {

LineRenderer::LineRenderer(usize max_lines)
{
    init(max_lines);
}

void LineRenderer::init(usize max_lines)
{
    util_assert(initialized == false, "already initialized");

    m_max_vertices = max_lines * 2;

    m_vao.init();

    m_ssbo.init(3, m_max_vertices * sizeof(Vertex));

    m_vertices.reserve(m_max_vertices);

    setup_shader();

    initialized = true;
}

LineRenderer::~LineRenderer()
{
    if (initialized) {
        initialized = false;
    }
}

void LineRenderer::add_line(glm::vec3 begin, glm::vec3 end, u32 color)
{
    util_assert(initialized == true, "not initialized");
    if (m_vertices.size() + 2 > m_max_vertices) {
        LOG_ERROR("Exceeding max number of lines, new lines are not being added");
        return;
    }
    m_vertices.push_back({ begin, color });
    m_vertices.push_back({ end, color });
}

void LineRenderer::add_line(glm::vec3 begin, glm::vec3 end, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_line(begin, end, Utils::Color::pack(color));
}

void LineRenderer::add_line(const glm::mat4& transform, glm::vec3 begin, glm::vec3 end, u32 color)
{
    util_assert(initialized == true, "not initialized");
    if (m_vertices.size() + 2 > m_max_vertices) {
        LOG_ERROR("Exceeding max number of lines, new lines are not being added");
        return;
    }
    m_vertices.push_back({ transform * glm::vec4(begin, 1.0), color });
    m_vertices.push_back({ transform * glm::vec4(end, 1.0), color });
}

void LineRenderer::add_line(const glm::mat4& transform, glm::vec3 begin, glm::vec3 end, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_line(transform, begin, end, Utils::Color::pack(color));
}

void LineRenderer::add_aabb(const Utils::AABB& aabb, u32 color)
{
    util_assert(initialized == true, "not initialized");

    glm::vec3 c[8] = {
        { aabb.min.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.min.y, aabb.min.z },
        { aabb.max.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.max.y, aabb.min.z },
        { aabb.min.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.min.y, aabb.max.z },
        { aabb.max.x, aabb.max.y, aabb.max.z },
        { aabb.min.x, aabb.max.y, aabb.max.z }
    };

    // Bottom face
    add_line(c[0], c[1], color);
    add_line(c[1], c[2], color);
    add_line(c[2], c[3], color);
    add_line(c[3], c[0], color);

    // Top face
    add_line(c[4], c[5], color);
    add_line(c[5], c[6], color);
    add_line(c[6], c[7], color);
    add_line(c[7], c[4], color);

    // Vertical pillars
    add_line(c[0], c[4], color);
    add_line(c[1], c[5], color);
    add_line(c[2], c[6], color);
    add_line(c[3], c[7], color);
}

void LineRenderer::add_aabb(const Utils::AABB& aabb, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_aabb(aabb, Utils::Color::pack(color));
}

void LineRenderer::add_ray(const Utils::Ray& ray, float length, u32 color)
{
    util_assert(initialized == true, "not initialized");
    glm::vec3 end = ray.position + (ray.direction * length);
    add_line(ray.position, end, color);
}

void LineRenderer::add_ray(const Utils::Ray& ray, float length, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_ray(ray, length, Utils::Color::pack(color));
}

void LineRenderer::add_circle(const glm::mat4& transform, f32 radius, u32 color)
{
    util_assert(initialized == true, "not initialized");
    add_circle(transform, radius, DEFAULT_CIRCLE_SEGMENTS, color);
}

void LineRenderer::add_circle(const glm::mat4& transform, f32 radius, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_circle(transform, radius, DEFAULT_CIRCLE_SEGMENTS, Utils::Color::pack(color));
}

void LineRenderer::add_circle(const glm::mat4& transform, f32 radius, u32 segments, u32 color)
{
    util_assert(initialized == true, "not initialized");

    float angle_step = glm::two_pi<float>() / static_cast<float>(segments);

    glm::vec4 first_local(radius, 0.0f, 0.0f, 1.0f);
    glm::vec3 first_world = transform * first_local;

    glm::vec3 prev_world = first_world;

    for (u32 i = 1; i <= segments + 1; ++i) {
        float angle = static_cast<float>(i) * angle_step;

        glm::vec4 local_pos(
            glm::cos(angle) * radius,
            glm::sin(angle) * radius,
            0.0f,
            1.0f);

        glm::vec3 current_world = transform * local_pos;

        add_line(prev_world, current_world, color);
        prev_world = current_world;
    }
}

void LineRenderer::add_circle(const glm::mat4& transform, f32 radius, u32 segments, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_circle(transform, radius, segments, Utils::Color::pack(color));
}

void LineRenderer::add_triangle(glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3, u32 color)
{
    util_assert(initialized == true, "not initialized");
    add_line(vert1, vert2, color);
    add_line(vert2, vert3, color);
    add_line(vert3, vert1, color);
}

void LineRenderer::add_triangle(glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3, glm::vec3 color)
{
    util_assert(initialized == true, "not initialized");
    add_triangle(vert1, vert2, vert3, Utils::Color::pack(color));
}

void LineRenderer::add_triangle(const glm::mat4& transform, glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3, u32 color)
{
    glm::vec3 vert1_t = transform * glm::vec4(vert1, 1.0);
    glm::vec3 vert2_t = transform * glm::vec4(vert2, 1.0);
    glm::vec3 vert3_t = transform * glm::vec4(vert3, 1.0);

    add_line(vert1_t, vert2_t, color);
    add_line(vert2_t, vert3_t, color);
    add_line(vert3_t, vert1_t, color);
}

void LineRenderer::add_triangle(const glm::mat4& transform, glm::vec3 vert1, glm::vec3 vert2, glm::vec3 vert3, glm::vec3 color)
{
    add_triangle(transform, vert1, vert2, vert3, Utils::Color::pack(color));
}

void LineRenderer::add_capsule(const Utils::Capsule& capsule, u32 color)
{
    const int segments = DEFAULT_CIRCLE_SEGMENTS;
    const f32 radius = capsule.radius;

    glm::vec3 axis = capsule.center_top - capsule.center_bottom;
    f32 length = glm::length(axis);

    // Default to pointing up if the capsule has zero height
    glm::vec3 up = (length > 0.0001f) ? (axis / length) : glm::vec3(0.0f, 1.0f, 0.0f);

    // Find a valid right vector by crossing 'up' with world UP or world RIGHT
    glm::vec3 right = glm::cross(up, glm::vec3(0.0f, 1.0f, 0.0f));
    if (glm::length(right) < 0.0001f) {
        right = glm::cross(up, glm::vec3(1.0f, 0.0f, 0.0f));
    }
    right = glm::normalize(right);
    glm::vec3 forward = glm::cross(up, right);

    f32 angle_step = glm::two_pi<f32>() / segments;

    for (int i = 0; i < segments; ++i) {
        f32 a1 = i * angle_step;
        f32 a2 = ((i + 1) % segments) * angle_step;

        glm::vec3 p1 = right * glm::cos(a1) * radius + forward * glm::sin(a1) * radius;
        glm::vec3 p2 = right * glm::cos(a2) * radius + forward * glm::sin(a2) * radius;

        add_line(capsule.center_bottom + p1, capsule.center_bottom + p2, color);
        add_line(capsule.center_top + p1, capsule.center_top + p2, color);

        if (i % (segments / 4) == 0) {
            add_line(capsule.center_bottom + p1, capsule.center_top + p1, color);
        }
    }

    int dome_segments = segments / 2;
    f32 dome_step = glm::pi<f32>() / dome_segments;

    for (int i = 0; i < dome_segments; ++i) {
        f32 a1 = i * dome_step;
        f32 a2 = (i + 1) * dome_step;

        f32 cos1 = glm::cos(a1), sin1 = glm::sin(a1);
        f32 cos2 = glm::cos(a2), sin2 = glm::sin(a2);

        // Top Dome (Right-Up Plane)
        glm::vec3 tr1 = right * cos1 * radius + up * sin1 * radius;
        glm::vec3 tr2 = right * cos2 * radius + up * sin2 * radius;
        add_line(capsule.center_top + tr1, capsule.center_top + tr2, color);

        // Top Dome (Forward-Up Plane)
        glm::vec3 tf1 = forward * cos1 * radius + up * sin1 * radius;
        glm::vec3 tf2 = forward * cos2 * radius + up * sin2 * radius;
        add_line(capsule.center_top + tf1, capsule.center_top + tf2, color);

        // Bottom Dome (Right-Down Plane)
        glm::vec3 br1 = right * cos1 * radius - up * sin1 * radius;
        glm::vec3 br2 = right * cos2 * radius - up * sin2 * radius;
        add_line(capsule.center_bottom + br1, capsule.center_bottom + br2, color);

        // Bottom Dome (Forward-Down Plane)
        glm::vec3 bf1 = forward * cos1 * radius - up * sin1 * radius;
        glm::vec3 bf2 = forward * cos2 * radius - up * sin2 * radius;
        add_line(capsule.center_bottom + bf1, capsule.center_bottom + bf2, color);
    }
}

void LineRenderer::add_capsule(const Utils::Capsule& capsule, glm::vec3 color)
{
    add_capsule(capsule, Utils::Color::pack(color));
}

void LineRenderer::draw(const Camera& camera)
{
    util_assert(initialized == true, "not initialized");
    if (m_vertices.size() == 0) {
        return;
    }
    m_vao.bind();
    m_shader.bind();

    // m_shader.set_vec3("color", color);
    m_shader.set_mat4("proj", camera.get_proj());
    m_shader.set_mat4("view", camera.get_view());

    void* ssbo_ptr = m_ssbo.get_ptr();
    memcpy(ssbo_ptr, &m_vertices[0], m_vertices.size() * sizeof(Vertex));

    // glDisable(GL_DEPTH_TEST);
    glEnable(GL_DEPTH_TEST);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo.get_id());
    glDrawArrays(GL_LINES, 0, m_vertices.size());

    // glEnable(GL_DEPTH_TEST);

    m_vertices.clear();
    m_ssbo.increment_frame();
}

void LineRenderer::setup_shader()
{
    ShaderInfoData<2> out;

    const char* file_name = "res/shaders/debug/lines.glsl";
    std::vector<char> text_shader_file;
    auto result = Utils::read_file(text_shader_file, file_name);
    if (!result) {
        util_error(std::format("Could not find file \"{}\"", file_name));
    }
    std::string_view text_shader_file_view = { text_shader_file.data(), text_shader_file.size() };

    // Vertex Shader
    out.data.at(0) = "#version 460 core\n";
    out.data.at(0) += get_lines_between_delims(text_shader_file_view, "// Vertex Begin", "// Vertex End");

    // Fragment Shader
    out.data.at(1) += "#version 460 core\n";
    out.data.at(1) += get_lines_between_delims(text_shader_file_view, "// Fragment Begin", "// Fragment End");

    out.populate_info();

    m_shader.init(out.info.data(), out.info.size());
}

} // namespace Renderer
