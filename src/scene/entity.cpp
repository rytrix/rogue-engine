#include "entity.hpp"

#include "scene.hpp"

#include "../app_data.hpp"

#include "../utils/file.hpp"

#include "../physics_jolt/interface.hpp"

Entity::Entity(Scene* scene, entt::entity entity)
    : m_scene(scene)
    , m_entity(entity)
{
}

bool Entity::valid()
{
    if (m_scene == nullptr) {
        return false;
    }
    if (m_entity == entt::null) {
        return false;
    }
    return m_scene->m_registry.valid(m_entity);
}

entt::entity Entity::get_id()
{
    return m_entity;
}

entt::registry& Entity::get_registry()
{
    return m_scene->m_registry;
}

Scene* Entity::get_scene()
{
    return m_scene;
}

void Entity::add_name(Entity entity, const char* name)
{
    entity.add_component<Utils::String>(name);
}

void Entity::add_transform(Entity entity, const Transform& transform)
{
    entity.add_component<Transform>(transform);
}

void Entity::add_mesh(Entity entity, const char* path)
{
    GlobalAppData* app_data = entity.m_scene->m_app_data;
    auto* mesh_cache = &app_data->m_mesh_cache;
    auto handle = mesh_cache->get_or_create(path, path, app_data);
    auto* mesh = mesh_cache->get(handle);
    auto result = mesh->get_result();
    if (result.type != Renderer::ModelResultEnum::Ok) {
        LOG_ERROR(std::format("Invalid Mesh with error: {}", result.error.c_str()));
        mesh_cache->destroy(handle);
        return;
    }

    if (mesh->m_has_bones) {
        auto& data = entity.add_component<Renderer::AnimationData>();
        for (auto& animation : mesh->m_animations) {
            data.data.emplace_back(animation.create_per_animation_data());
        }
    }

    entity.add_component<Renderer::Mesh*>(mesh);
    entity.get_scene()->m_mesh_instance_draw_cache_needs_update = true;
}

void Entity::add_static_body(Entity entity)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");
    auto physics_info = Physics::create_static_body(entity);
    entity.add_component<Physics::PhysicsInfo>(physics_info);
    entity.get_scene()->m_physics_needs_optimize = true;
}

void Entity::add_dynamic_body(Entity entity, JPH::Ref<JPH::Shape> shape)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");
    auto physics_info = Physics::create_dynamic_body(entity, shape);
    entity.add_component<Physics::PhysicsInfo>(physics_info);
    entity.get_scene()->m_physics_needs_optimize = true;
}

void Entity::add_convex_hull_body(Entity entity)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");
    auto physics_info = Physics::create_convex_hull(entity);
    entity.add_component<Physics::PhysicsInfo>(physics_info);
    entity.get_scene()->m_physics_needs_optimize = true;
}

void Entity::add_pbr_directional_light(Entity entity, Renderer::Light::Pbr::Directional& info)
{
    entity.add_component<Renderer::Light::Pbr::Directional>(info);
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::add_pbr_directional_light_shadow(Entity entity)
{
    entity.add_component<Renderer::Light::Pbr::DirectionalShadow>().init();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::add_pbr_point_light(Entity entity, Renderer::Light::Pbr::Point& info)
{
    entity.add_component<Renderer::Light::Pbr::Point>(info);
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::add_pbr_point_light_shadow(Entity entity)
{
    entity.add_component<Renderer::Light::Pbr::PointShadow>().init();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::add_pbr_spot_light(Entity entity, Renderer::Light::Pbr::Spot& info)
{
    info.calculate_cutoffs();
    entity.add_component<Renderer::Light::Pbr::Spot>(info);
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::add_pbr_spot_light_shadow(Entity entity)
{
    entity.add_component<Renderer::Light::Pbr::SpotShadow>().init();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::to_json(nlohmann::json& json, Entity entity)
{
    if (entity.has_component<Utils::String>()) {
        json["name"] = entity.get_component<Utils::String>().c_str();
    }

    if (entity.has_component<Transform>()) {
        Transform transform = entity.get_component<Transform>();
        json["transform"]["position"] = {
            transform.get_position().x,
            transform.get_position().y,
            transform.get_position().z,
        };

        json["transform"]["rotation"] = {
            transform.get_euler_angles().x,
            transform.get_euler_angles().y,
            transform.get_euler_angles().z,
        };

        json["transform"]["scale"] = {
            transform.get_scale().x,
            transform.get_scale().y,
            transform.get_scale().z,
        };
    }

    if (entity.has_component<Renderer::Mesh*>()) {
        json["mesh"] = entity.get_component<Renderer::Mesh*>()->m_path.c_str();
    }

    if (entity.has_component<Physics::PhysicsInfo>()) {
        auto& info = entity.get_component<Physics::PhysicsInfo>();
        if (info.m_type == Physics::PhysicsType::Mesh) {
            json["physics_body"] = "Mesh";
        } else if (info.m_type == Physics::PhysicsType::ConvexHull) {
            json["physics_body"] = "ConvexHull";
        }
    }

    if (entity.has_component<Renderer::Light::Pbr::Directional>()) {
        auto& info = entity.get_component<Renderer::Light::Pbr::Directional>();
        json["directional_light"]["direction"] = {
            info.direction.x,
            info.direction.y,
            info.direction.z,
        };

        json["directional_light"]["color"] = {
            info.color.x,
            info.color.y,
            info.color.z,
        };

        if (entity.has_component<Renderer::Light::Pbr::DirectionalShadow>()) {
            json["directional_light"]["shadow"] = true;
        }
    }

    if (entity.has_component<Renderer::Light::Pbr::Point>()) {
        auto& info = entity.get_component<Renderer::Light::Pbr::Point>();
        json["point_light"]["position"] = {
            info.position.x,
            info.position.y,
            info.position.z,
        };

        json["point_light"]["color"] = {
            info.color.x,
            info.color.y,
            info.color.z,
        };

        if (entity.has_component<Renderer::Light::Pbr::PointShadow>()) {
            json["point_light"]["shadow"] = true;
        }
    }

    if (entity.has_component<Renderer::Light::Pbr::Spot>()) {
        auto& info = entity.get_component<Renderer::Light::Pbr::Spot>();
        json["spot_light"]["position"] = {
            info.position.x,
            info.position.y,
            info.position.z,
        };

        json["spot_light"]["direction"] = {
            info.direction.x,
            info.direction.y,
            info.direction.z,
        };

        json["spot_light"]["color"] = {
            info.color.x,
            info.color.y,
            info.color.z,
        };

        json["spot_light"]["cutoff_inner"] = info.inner_cutoff_degrees;
        json["spot_light"]["cutoff_outer"] = info.outer_cutoff_degrees;

        if (entity.has_component<Renderer::Light::Pbr::SpotShadow>()) {
            json["spot_light"]["shadow"] = true;
        }
    }
}

void Entity::from_json(nlohmann::json& json, Entity entity)
{
    if (json.contains("name")) {
        if (entity.has_component<Utils::String>()) {
            entity.remove_component<Utils::String>();
        }
        auto name = json["name"].get_ref<const std::string&>().c_str();
        Entity::add_name(entity, name);
    }

    if (json.contains("transform")) {
        Transform transform;
        if (entity.has_component<Transform>()) {
            entity.remove_component<Transform>();
        }

        if (json["transform"].contains("position") && json["transform"]["position"].is_array() && json["transform"]["position"].size() == 3) {
            auto position = json["transform"]["position"];
            transform.set_position({ position[0].get<float>(),
                position[1].get<float>(),
                position[2].get<float>() });
        }

        if (json["transform"].contains("rotation") && json["transform"]["rotation"].is_array() && json["transform"]["rotation"].size() == 3) {
            auto rotation = json["transform"]["rotation"];
            transform.set_euler_angles({ rotation[0].get<float>(),
                rotation[1].get<float>(),
                rotation[2].get<float>() });
        }

        if (json["transform"].contains("scale")) { // && json["transform"]["scale"].is_array() && json["transform"]["scale"].size() == 3) {
            auto scale = json["transform"]["scale"];
            transform.set_scale({ scale[0].get<float>(),
                scale[1].get<float>(),
                scale[2].get<float>() });
        }

        Entity::add_transform(entity, transform);
    }

    if (json.contains("mesh")) {
        if (entity.has_component<Renderer::Mesh*>()) {
            entity.remove_component<Renderer::Mesh*>();
        }
        if (entity.has_component<Renderer::AnimationData>()) {
            entity.remove_component<Renderer::AnimationData>();
        }

        if (json["mesh"].is_string()) {
            auto name = json["mesh"].get_ref<const std::string&>().c_str();
            Entity::add_mesh(entity, name);
        }
    }

    if (json.contains("physics_body") && json["physics_body"].is_string()) {
        if (entity.has_component<Physics::PhysicsInfo>()) {
            entity.remove_component<Physics::PhysicsInfo>();
        }

        auto physics_body = json["physics_body"].get_ref<const std::string&>();

        if (physics_body == "Mesh") {
            Entity::add_static_body(entity);
        } else if (physics_body == "ConvexHull") {
            Entity::add_convex_hull_body(entity);
        }
    }

    if (json.contains("directional_light")) {
        if (entity.has_component<Renderer::Light::Pbr::Directional>()) {
            entity.remove_component<Renderer::Light::Pbr::Directional>();
        }
        if (entity.has_component<Renderer::Light::Pbr::DirectionalShadow>()) {
            entity.remove_component<Renderer::Light::Pbr::DirectionalShadow>();
        }

        Renderer::Light::Pbr::Directional info {};

        if (json["directional_light"].contains("direction") && json["directional_light"]["direction"].is_array() && json["directional_light"]["direction"].size() == 3) {
            auto info_json = json["directional_light"]["direction"];
            info.direction = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        if (json["directional_light"].contains("color") && json["directional_light"]["color"].is_array() && json["directional_light"]["color"].size() == 3) {
            auto info_json = json["directional_light"]["color"];
            info.color = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        Entity::add_pbr_directional_light(entity, info);

        if (json["directional_light"].contains("shadow")) {
            Entity::add_pbr_directional_light_shadow(entity);
        }
    }

    if (json.contains("point_light")) {
        if (entity.has_component<Renderer::Light::Pbr::Point>()) {
            entity.remove_component<Renderer::Light::Pbr::Point>();
        }
        if (entity.has_component<Renderer::Light::Pbr::PointShadow>()) {
            entity.remove_component<Renderer::Light::Pbr::PointShadow>();
        }

        Renderer::Light::Pbr::Point info {};

        if (json["point_light"].contains("position") && json["point_light"]["position"].is_array() && json["point_light"]["position"].size() == 3) {
            auto info_json = json["point_light"]["position"];
            info.position = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        if (json["point_light"].contains("color") && json["point_light"]["color"].is_array() && json["point_light"]["color"].size() == 3) {
            auto info_json = json["point_light"]["color"];
            info.color = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        Entity::add_pbr_point_light(entity, info);

        if (json["point_light"].contains("shadow")) {
            Entity::add_pbr_point_light_shadow(entity);
        }
    }

    if (json.contains("spot_light")) {
        if (entity.has_component<Renderer::Light::Pbr::Spot>()) {
            entity.remove_component<Renderer::Light::Pbr::Spot>();
        }
        if (entity.has_component<Renderer::Light::Pbr::SpotShadow>()) {
            entity.remove_component<Renderer::Light::Pbr::SpotShadow>();
        }

        Renderer::Light::Pbr::Spot info {};

        if (json["spot_light"].contains("position") && json["spot_light"]["position"].is_array() && json["spot_light"]["position"].size() == 3) {
            auto info_json = json["spot_light"]["position"];
            info.position = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        if (json["spot_light"].contains("direction") && json["spot_light"]["direction"].is_array() && json["spot_light"]["direction"].size() == 3) {
            auto info_json = json["spot_light"]["direction"];
            info.direction = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        if (json["spot_light"].contains("color") && json["spot_light"]["color"].is_array() && json["spot_light"]["color"].size() == 3) {
            auto info_json = json["spot_light"]["color"];
            info.color = {
                info_json[0].get<float>(),
                info_json[1].get<float>(),
                info_json[2].get<float>()
            };
        }

        if (json["spot_light"].contains("cutoff_inner")) {
            auto cutoff = json["spot_light"]["cutoff_inner"];
            info.inner_cutoff_degrees = cutoff;
        }

        if (json["spot_light"].contains("cutoff_outer")) {
            auto cutoff = json["spot_light"]["cutoff_outer"];
            info.outer_cutoff_degrees = cutoff;
        }

        Entity::add_pbr_spot_light(entity, info);

        if (json["spot_light"].contains("shadow")) {
            Entity::add_pbr_spot_light_shadow(entity);
        }
    }
}
