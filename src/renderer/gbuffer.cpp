#include "gbuffer.hpp"

namespace Renderer {

void GBuffer::init(int screen_width, int screen_height)
{
    util_assert(initialized == false, "already initialized");

    m_buffer_width = screen_width;
    m_buffer_height = screen_height;

    m_buffer.init();

    Renderer::TextureInfo texture_info;
    texture_info.memory_info.size = Renderer::TextureSize { .width = screen_width, .height = screen_height, .depth = 0 };
    texture_info.internal_format = GL_RGBA16F;
    texture_info.mipmaps = GL_FALSE;
    m_position.init(texture_info);
    m_buffer.bind_texture(GL_COLOR_ATTACHMENT0, m_position.get_id(), 0);

    m_normal.init(texture_info);
    m_buffer.bind_texture(GL_COLOR_ATTACHMENT1, m_normal.get_id(), 0);

    texture_info.internal_format = GL_RGBA16F;
    m_albedo.init(texture_info);
    m_buffer.bind_texture(GL_COLOR_ATTACHMENT2, m_albedo.get_id(), 0);

    m_depth.init();
    m_depth.buffer_storage(GL_DEPTH_COMPONENT24, screen_width, screen_height);

    m_buffer.bind_renderbuffer(GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_depth.get_id());

    std::array<u32, 3> attachments = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
    m_buffer.bind_draw_buffers(attachments.size(), attachments.data());

    initialized = true;
}

GBuffer::~GBuffer()
{
    initialized = false;
}

void GBuffer::reinit(int screen_width, int screen_height)
{
    // TODO this is UB...
    this->~GBuffer();
    init(screen_width, screen_height);
}

void GBuffer::bind()
{
    util_assert(initialized == true, "not initialized");
    m_buffer.bind();
}

void GBuffer::set_uniforms(Renderer::Shader& shader)
{
    util_assert(initialized == true, "not initialized");

    GLuint texture_unit = Texture::get_texture_unit();
    m_position.bind(texture_unit);
    shader.set_int("gPosition", static_cast<i32>(texture_unit));

    texture_unit = Texture::get_texture_unit();
    m_normal.bind(texture_unit);
    shader.set_int("gNormal", static_cast<i32>(texture_unit));

    texture_unit = Texture::get_texture_unit();
    m_albedo.bind(texture_unit);
    shader.set_int("gAlbedoSpec", static_cast<int>(texture_unit));
}

void GBuffer::unbind()
{
    util_assert(initialized == true, "not initialized");
    m_buffer.unbind();
}

void GBuffer::blit_depth_buffer()
{
    util_assert(initialized == true, "not initialized");
    glBlitNamedFramebuffer(m_buffer.get_id(), 0, 0, 0, m_buffer_width, m_buffer_height, 0, 0, m_buffer_width, m_buffer_height, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
}

} // namespace Renderer
