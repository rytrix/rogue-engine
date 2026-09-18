#pragma once

#include "../event.hpp"

class System {
public:
    virtual ~System() = default;
    virtual void on_event([[maybe_unused]] Event& event) { };
    virtual void update() { };
    virtual void draw() { };
};
