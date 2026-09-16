#include "transform.hpp"

namespace Utils {

void Transform::set_position(const glm::vec3& position)
{
    m_position = position;
    needs_update = true;
}

void Transform::translate(const glm::vec3& delta)
{
    m_position += delta;
    needs_update = true;
}

void Transform::set_rotation(const glm::quat& rotation)
{
    m_rotation = rotation;
    needs_update = true;
}

void Transform::rotate(float angle_degrees, const glm::vec3& axis)
{
    m_rotation = glm::rotate(m_rotation, glm::radians(angle_degrees), axis);
    needs_update = true;
}

void Transform::set_euler_angles(const glm::vec3& degrees)
{
    m_rotation = glm::quat(glm::radians(degrees));
    needs_update = true;
}

glm::vec3 Transform::get_euler_angles() const
{
    return glm::degrees(glm::eulerAngles(m_rotation));
}

void Transform::set_scale(const glm::vec3& scale)
{
    m_scale = scale;
    needs_update = true;
}

[[nodiscard]] glm::vec3 Transform::get_position() const
{
    return m_position;
}

[[nodiscard]] glm::quat Transform::get_rotation() const
{
    return m_rotation;
}

[[nodiscard]] glm::vec3 Transform::get_scale() const
{
    return m_scale;
}

const glm::mat4& Transform::get_model_matrix()
{
    if (needs_update) {
        m_model_matrix = glm::translate(glm::mat4(1.0F), m_position)
            * glm::mat4_cast(m_rotation)
            * glm::scale(glm::mat4(1.0F), m_scale);
        needs_update = false;
    }
    return m_model_matrix;
}

glm::mat4& Transform::get_model_matrix_ref()
{
    return m_model_matrix;
}

void Transform::set_model(const glm::mat4& model)
{
    m_model_matrix = model;
    needs_update = false;
}

} // namespace Utils
