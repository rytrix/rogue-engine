#pragma once

namespace Utils {

class Transform {
public:
    Transform() = default;

    void set_position(const glm::vec3& position);
    void translate(const glm::vec3& delta);
    void set_rotation(const glm::quat& rotation);
    void rotate(float angle_degrees, const glm::vec3& axis);
    void set_euler_angles(const glm::vec3& degrees);
    void set_scale(const glm::vec3& scale);

    [[nodiscard]] glm::vec3 get_position() const;
    [[nodiscard]] glm::quat get_rotation() const;
    [[nodiscard]] glm::vec3 get_euler_angles() const;
    [[nodiscard]] glm::vec3 get_scale() const;

    // Updates the model matrix
    [[nodiscard]] const glm::mat4& get_model_matrix();

    [[nodiscard]] glm::mat4& get_model_matrix_ref();
    void set_model(const glm::mat4& model);

private:
    glm::vec3 m_position = { 0.0F, 0.0F, 0.0F };
    glm::quat m_rotation = glm::identity<glm::quat>();
    glm::vec3 m_scale = { 1.0F, 1.0F, 1.0F };

    bool needs_update = true;
    glm::mat4 m_model_matrix = glm::mat4(1.0F);
};


} // namespace Utils
