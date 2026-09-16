#ifndef ENTITY_HPP
#define ENTITY_HPP

#include "../utils/transform.hpp"

class Scene;
class Entity;
namespace Renderer::Light::Pbr {
    class Directional;
    class Point;
    class Spot;
}

class Entity {
public:
    Entity() = default;
    Entity(Scene* scene, entt::entity entity);

    template <typename T, typename... Args>
    T& add_component(Args&&... args);

    template <typename T>
    void remove_component();

    template <typename T>
    T& get_component();

    template <typename T>
    bool has_component();

    bool valid();

    entt::entity get_id();
    entt::registry& get_registry();
    Scene* get_scene();

    // Helper functions
    static void add_name(Entity entity, const char* name);
    static void add_transform(Entity entity, const Utils::Transform& transform);
    static void add_mesh(Entity entity, const char* path);
    static void add_static_body(Entity entity);
    // static void add_dynamic_body(Entity entity, JPH::Ref<JPH::Shape> shape);
    static void add_convex_hull_body(Entity entity);
    static void add_pbr_directional_light(Entity entity, Renderer::Light::Pbr::Directional& info);
    static void add_pbr_directional_light_shadow(Entity entity);
    static void add_pbr_point_light(Entity entity, Renderer::Light::Pbr::Point& info);
    static void add_pbr_point_light_shadow(Entity entity);
    static void add_pbr_spot_light(Entity entity, Renderer::Light::Pbr::Spot& info);
    static void add_pbr_spot_light_shadow(Entity entity);

    static void to_json(nlohmann::json& json, Entity entity);
    static void from_json(nlohmann::json& json, Entity entity);

private:
    Scene* m_scene = nullptr;
    entt::entity m_entity = entt::null;
};

#endif

#ifdef ENTITY_IMPL
#include "entity_impl.hpp"
#endif
