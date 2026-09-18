#define ENTITY_IMPL
#include "entity.hpp"

#include "../app_data.hpp"

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

void Entity::remove_name(Entity entity)
{
    entity.remove_component<Utils::String>();
}

void Entity::add_transform(Entity entity, const Utils::Transform& transform)
{
    entity.add_component<Utils::Transform>(transform);
}

void Entity::remove_transform(Entity entity)
{
    entity.remove_component<Utils::Transform>();
}

void Entity::add_mesh(Entity entity, const char* path)
{
    auto* mesh_cache = g_app_data->m_mesh_cache;
    auto handle = mesh_cache->get_or_create(path, path);
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

void Entity::remove_mesh(Entity entity)
{
    entity.remove_component<Renderer::Mesh*>();
    entity.remove_component<Renderer::AnimationData>();
}

void Entity::add_static_body(Entity entity)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");

    auto entity_info = entity.get_scene()->m_physics_engine->create_mesh_body(entity);
    if (!entity_info.m_valid) {
        LOG_ERROR("Failed to create mesh body");
        return;
    }
    entity.add_component<PhysicsBox3d::EntityInfo>(entity_info);
}

void Entity::add_convex_hull_body(Entity entity)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");

    auto entity_info = entity.get_scene()->m_physics_engine->create_hull_body(entity);
    if (!entity_info.m_valid) {
        LOG_ERROR("Failed to create convex hull body");
        return;
    }
    entity.add_component<PhysicsBox3d::EntityInfo>(entity_info);
}

void Entity::add_box_hull_body(Entity entity, const PhysicsBox3d::BoxHullInfo& info)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");

    auto entity_info = entity.get_scene()->m_physics_engine->create_box_body(entity, info);
    if (!entity_info.m_valid) {
        LOG_ERROR("Failed to create box hull body");
        return;
    }
    entity.add_component<PhysicsBox3d::EntityInfo>(entity_info);
}

void Entity::add_capsule_body(Entity entity, const PhysicsBox3d::CapsuleInfo& info)
{
    util_assert(entity.has_component<Renderer::Mesh*>() == true, "Cannot add physics to an entity without a mesh");

    auto entity_info = entity.get_scene()->m_physics_engine->create_capsule_body(entity, info);
    if (!entity_info.m_valid) {
        LOG_ERROR("Failed to create capsule body");
        return;
    }
    entity.add_component<PhysicsBox3d::EntityInfo>(entity_info);
}

void Entity::remove_physics_body(Entity entity)
{
    if (entity.has_component<PhysicsBox3d::EntityInfo>()) {
        auto physics_info = entity.get_component<PhysicsBox3d::EntityInfo>();
        entity.get_scene()->m_physics_engine->remove_body(physics_info.m_id);
        entity.remove_component<PhysicsBox3d::EntityInfo>();
    }
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

void Entity::remove_pbr_directional_light(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::Directional>();
    entity.remove_component<Renderer::Light::Pbr::DirectionalShadow>();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::remove_pbr_directional_light_shadow(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::DirectionalShadow>();
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

void Entity::remove_pbr_point_light(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::Point>();
    entity.remove_component<Renderer::Light::Pbr::PointShadow>();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::remove_pbr_point_light_shadow(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::PointShadow>();
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

void Entity::remove_pbr_spot_light(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::Spot>();
    entity.remove_component<Renderer::Light::Pbr::SpotShadow>();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::remove_pbr_spot_light_shadow(Entity entity)
{
    entity.remove_component<Renderer::Light::Pbr::PointShadow>();
    entity.get_scene()->m_shaders_need_update = true;
}

void Entity::to_json(nlohmann::json& json, Entity entity)
{
    if (entity.has_component<Utils::String>()) {
        json["name"] = entity.get_component<Utils::String>().c_str();
    }

    if (entity.has_component<Utils::Transform>()) {
        Utils::Transform transform = entity.get_component<Utils::Transform>();
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

    if (entity.has_component<PhysicsBox3d::EntityInfo>()) {
        auto& info = entity.get_component<PhysicsBox3d::EntityInfo>();
        if (info.m_type == PhysicsBox3d::Type::Mesh) {
            json["physics_body"] = "Mesh";
        } else if (info.m_type == PhysicsBox3d::Type::ConvexHull) {
            json["physics_body"] = "ConvexHull";
        } else if (info.m_type == PhysicsBox3d::Type::BoxHull) {
            json["physics_body"] = "BoxHull";
            json["physics_info"]["center"] = {
                info.m_box_hull_info.center.x,
                info.m_box_hull_info.center.y,
                info.m_box_hull_info.center.z,
            };
            json["physics_info"]["extent"] = {
                info.m_box_hull_info.extent.x,
                info.m_box_hull_info.extent.y,
                info.m_box_hull_info.extent.z,
            };
        } else if (info.m_type == PhysicsBox3d::Type::Capsule) {
            json["physics_body"] = "Capsule";
            json["physics_info"]["center-bottom"] = {
                info.m_capsule_info.center_bottom.x,
                info.m_capsule_info.center_bottom.y,
                info.m_capsule_info.center_bottom.z,
            };
            json["physics_info"]["center-top"] = {
                info.m_capsule_info.center_top.x,
                info.m_capsule_info.center_top.y,
                info.m_capsule_info.center_top.z,
            };
            json["physics_info"]["radius"] = info.m_capsule_info.radius;
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
        Utils::Transform transform;
        if (entity.has_component<Utils::Transform>()) {
            entity.remove_component<Utils::Transform>();
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
        if (entity.has_component<PhysicsBox3d::EntityInfo>()) {
            entity.remove_component<PhysicsBox3d::EntityInfo>();
        }

        auto physics_body = json["physics_body"].get_ref<const std::string&>();

        if (physics_body == "Mesh") {
            Entity::add_static_body(entity);
        } else if (physics_body == "ConvexHull") {
            Entity::add_convex_hull_body(entity);
        } else if (physics_body == "BoxHull") {
            if (json.contains("physics_info")
                && json["physics_info"].contains("center")
                && json["physics_info"]["center"].is_array()
                && json["physics_info"]["center"].size() == 3
                && json["physics_info"].contains("extent")
                && json["physics_info"]["extent"].is_array()
                && json["physics_info"]["extent"].size() == 3) {

                PhysicsBox3d::BoxHullInfo info;
                auto center_json = json["physics_info"]["center"];
                auto extent_json = json["physics_info"]["extent"];

                info.center = {
                    center_json[0].get<float>(),
                    center_json[1].get<float>(),
                    center_json[2].get<float>()
                };
                info.extent = {
                    extent_json[0].get<float>(),
                    extent_json[1].get<float>(),
                    extent_json[2].get<float>()
                };

                Entity::add_box_hull_body(entity, info);
            }
        } else if (physics_body == "Capsule") {
            if (json.contains("physics_info")
                && json["physics_info"].contains("center-bottom")
                && json["physics_info"]["center-bottom"].is_array()
                && json["physics_info"]["center-bottom"].size() == 3
                && json["physics_info"].contains("center-top")
                && json["physics_info"]["center-top"].is_array()
                && json["physics_info"]["center-top"].size() == 3
                && json["physics_info"].contains("radius")
                && json["physics_info"]["radius"].is_number()) {

                PhysicsBox3d::CapsuleInfo info;
                auto bottom_json = json["physics_info"]["center-bottom"];
                auto top_json = json["physics_info"]["center-top"];
                auto radius_json = json["physics_info"]["radius"];

                info.center_bottom = {
                    bottom_json[0].get<float>(),
                    bottom_json[1].get<float>(),
                    bottom_json[2].get<float>()
                };
                info.center_top = {
                    top_json[0].get<float>(),
                    top_json[1].get<float>(),
                    top_json[2].get<float>()
                };
                info.radius = radius_json.get<float>();

                Entity::add_capsule_body(entity, info);
            }
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
