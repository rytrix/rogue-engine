#pragma once

#include "renderer.hpp"

#include "scene/scene.hpp"

#include "app_data.hpp"

class App : public NoCopyNoMove {
public:
    App();
    ~App();

    void run();

private:
    void fps_counter();
    void spawn_300_cubes();

    bool m_vsync = true;
    bool m_draw_bodies = false;
    u32 m_fps = 0;

    GlobalAppData m_app_data;

    Scene* m_scene = nullptr;
};
