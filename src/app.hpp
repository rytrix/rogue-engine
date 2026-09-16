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

    // Used for allocations that are only done once
    std::pmr::monotonic_buffer_resource m_default_allocator;

    template <typename T, typename... Args>
    T* construct(Args&&... args)
    {
        T* ptr = (T*)m_default_allocator.allocate(sizeof(T));
        return std::construct_at(ptr, std::forward<Args>(args)...);
    }

    void construct_globals();
    void destroy_globals();

    Scene* m_scene = nullptr;
};
