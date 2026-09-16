#pragma once

#include "../scene/resource_manager.hpp"

namespace Renderer {

class Shader;

struct RandomSamplingTexture : public NoCopyNoMove {
    static RandomSamplingTexture create(i32 window_size, i32 filter_size, i32 radius);

    [[nodiscard]] Handle get_handle() const { return m_handle; }
    [[nodiscard]] i32 get_window_size() const { return m_window_size; }
    [[nodiscard]] i32 get_filter_size() const { return m_filter_size; }

    void bind_uniforms(Shader& shader, const char* uniform_name);

    i32 m_radius;

private:
    RandomSamplingTexture(Handle handle, i32 window_size, i32 filter_size, i32 radius);
    static Handle create_random_sampling_texture(i32 window_size, i32 filter_size);

    Handle m_handle;
    i32 m_window_size;
    i32 m_filter_size;
};

} // namespace Renderer
