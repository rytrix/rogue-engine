#pragma once

#include "scene.hpp"

template <typename T, typename... Args>
T& Entity::add_component(Args&&... args)
{
    remove_component<T>();
    // util_assert(has_component<T>() == false, std::format("Entity already has component \"{}\"", typeid(T).name()));
    return m_scene->m_registry.emplace<T>(m_entity, std::forward<Args>(args)...);
}

template <typename T>
void Entity::remove_component()
{
    if (has_component<T>()) {
        m_scene->m_registry.remove<T>(m_entity);
    }
}

template <typename T>
T& Entity::get_component()
{
    return m_scene->m_registry.get<T>(m_entity);
}

template <typename T>
bool Entity::has_component()
{
    if (!valid()) {
        return false;
    }
    return m_scene->m_registry.all_of<T>(m_entity);
}
