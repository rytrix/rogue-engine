#pragma once

#include "system.hpp"

class EventBus {
public:
    void add_system(System* system);

    void handle_event(Event event);
    void update();
    void draw();

private:
    std::vector<System*> m_systems;
};
