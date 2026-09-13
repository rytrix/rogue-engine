#include "shadowmap.hpp"

namespace Renderer {

ShadowMap::~ShadowMap()
{
    initialized = false;
}

void ShadowMap::init()
{
    init_internal(MapType::Default);
}

void ShadowMap::init(i32 width, i32 height)
{
    m_shadow_width = width;
    m_shadow_height = height;
    init_internal(MapType::Default);
}

void ShadowMap::init_cascade(i32 cascades)
{
    m_cascades = cascades;
    init_internal(MapType::Cascade);
}

void ShadowMap::init_cascade(i32 width, i32 height, i32 cascades)
{
    m_shadow_width = width;
    m_shadow_height = height;
    m_cascades = cascades;
    init_internal(MapType::Cascade);
}

void ShadowMap::init_cubemap()
{
    init_internal(MapType::CubeMap);
}

void ShadowMap::init_cubemap(i32 width, i32 height)
{
    m_shadow_width = width;
    m_shadow_height = height;
    init_internal(MapType::CubeMap);
}

void ShadowMap::init_internal(MapType map_type)
{
    util_assert(initialized == false, "already initialized");

    Renderer::TextureInfo shadowmap_info;
    if (map_type == MapType::CubeMap) {
        shadowmap_info.dimensions = GL_TEXTURE_CUBE_MAP;
    } else if (map_type == MapType::Cascade) {
        shadowmap_info.dimensions = GL_TEXTURE_2D_ARRAY;
    } else {
        shadowmap_info.dimensions = GL_TEXTURE_2D;
    }
    shadowmap_info.memory_info.size = Renderer::TextureSize { .width = m_shadow_width, .height = m_shadow_height, .depth = m_cascades };
    shadowmap_info.internal_format = GL_DEPTH_COMPONENT24;
    shadowmap_info.wrap_s = GL_CLAMP_TO_BORDER;
    shadowmap_info.wrap_t = GL_CLAMP_TO_BORDER;
    shadowmap_info.wrap_r = GL_CLAMP_TO_BORDER;
    shadowmap_info.min_filter = GL_NEAREST;
    shadowmap_info.mag_filter = GL_NEAREST;
    m_texture.init(shadowmap_info);

    m_framebuffer.init();
    m_framebuffer.bind_texture(GL_DEPTH_ATTACHMENT, m_texture.get_id(), 0);
    m_framebuffer.bind_draw_buffer(GL_NONE);
    m_framebuffer.bind_read_buffer(GL_NONE);

    initialized = true;
}

void ShadowMap::bind()
{
    util_assert(initialized == true, "not initialized");
    m_framebuffer.bind();
}

void ShadowMap::bind_texture_layer(GLint layer)
{
    util_assert(initialized == true, "not initialized");
    m_framebuffer.bind_texture(GL_DEPTH_ATTACHMENT, m_texture.get_id(), 0, layer);
}

void ShadowMap::unbind()
{
    util_assert(initialized == true, "not initialized");
    m_framebuffer.bind_texture(GL_DEPTH_ATTACHMENT, m_texture.get_id(), 0);
    m_framebuffer.unbind();
}

[[nodiscard]] i32 ShadowMap::get_width() const
{
    util_assert(initialized == true, "not initialized");
    return m_shadow_width;
}

[[nodiscard]] i32 ShadowMap::get_height() const
{
    util_assert(initialized == true, "not initialized");
    return m_shadow_height;
}

[[nodiscard]] Texture& ShadowMap::get_texture()
{
    util_assert(initialized == true, "not initialized");
    return m_texture;
}

[[nodiscard]] bool ShadowMap::is_initialized() const
{
    return initialized;
}

} // namespace Renderer
