#pragma once

#include "../utils/math/ray.hpp"
#include "../scene/entity.hpp"
#include "../utils/transform.hpp"

#include "types.hpp"

class Scene;
namespace Utils {
    class Transform;
}
class Entity;

namespace PhysicsBox3d {

struct EntityInfo {
    b3BodyId m_id;
    MotionType m_motion_type;
    Type m_type;

    bool m_should_debug_draw = false;

    b3ShapeId m_shape;
    Entity m_entity;
    bool m_valid = false;
};

class Engine : public NoCopyNoMove {
public:
    Engine(Scene* scene);
    ~Engine();

    void update(float delta_time);

    void set_body_transform(b3BodyId body, const glm::vec3& pos, const glm::quat& rot);
    void set_body_transform(b3BodyId body, const Utils::Transform& transform);

    void get_body_transform(Utils::Transform& transform, b3BodyId body);
    [[nodiscard]] glm::vec3 get_body_pos(b3BodyId body);
    [[nodiscard]] glm::quat get_body_rot(b3BodyId body);

    void remove_body(b3BodyId body);

    std::optional<b3BodyId> ray_cast(Utils::Ray ray, float max_distance);
 
    [[nodiscard]] EntityInfo create_mesh_body(Entity entity);
    [[nodiscard]] EntityInfo create_hull_body(Entity entity);

    [[nodiscard]] EntityInfo create_box_body(Entity entity, const BoxHullInfo& info);
    [[nodiscard]] EntityInfo create_capsule_body(Entity entity);

private:
    b3WorldId m_world_id;
    float m_dt_accumulator = 0.0F;

    Scene* m_scene;
};

} // namespace PhysicsBox3d
