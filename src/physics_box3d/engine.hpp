#pragma once

#include <box3d/box3d.h>

#include "../scene/entity.hpp"
#include "../scene/transform.hpp"

class Transform;

namespace PhysicsBox3d {

enum class Type {
    Mesh,
    Shape,
    ConvexHull
};

struct EntityInfo {
    b3BodyId m_id;
    b3BodyType m_body_state;
    Type m_type;

    bool m_should_debug_draw = false;

    b3ShapeId m_shape;
    Entity m_entity;
};

class Engine : public NoCopyNoMove {
public:
    Engine(Scene* scene);
    ~Engine();

    void update(float delta_time);

    void get_body_transform(Transform& transform, b3BodyId body);

    [[nodiscard]] b3BodyId add_body(const b3BodyDef* body);
    void remove_body(const b3BodyId body);
 
    [[nodiscard]] EntityInfo create_mesh_body(Entity entity);

    [[nodiscard]] EntityInfo create_hull_body(Entity entity);

private:
    b3WorldId m_world_id;
    float m_dt_accumulator = 0.0F;

    Scene* m_scene;
};

} // namespace PhysicsBox3d
