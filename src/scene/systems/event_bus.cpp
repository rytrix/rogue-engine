#include "event_bus.hpp"

void EventBus::add_system(System* system)
{
    m_systems.push_back(system);
}

void EventBus::handle_event(Event event)
{
    for (u32 i = 0; i < m_systems.size(); i++) {
        if (event.m_consumed) {
            return;
        }
        m_systems[i]->on_event(event);
    }
}

void EventBus::update()
{
    for (u32 i = 0; i < m_systems.size(); i++) {
        m_systems[i]->update();
    }
}

void EventBus::draw()
{
    for (u32 i = 0; i < m_systems.size(); i++) {
        m_systems[i]->draw();
    }
}
