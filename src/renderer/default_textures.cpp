#include "default_textures.hpp"

#include "texture.hpp"

namespace Renderer {

DefaultTextures::DefaultTextures(TextureCache* cache)
{
    init(cache);
}

DefaultTextures::~DefaultTextures()
{
    if (m_cache != nullptr) {
        m_cache->destroy(m_albedo);
        m_cache->destroy(m_metallic);
        m_cache->destroy(m_normal);
        m_cache = nullptr;
    }
    initialized = false;
}

void DefaultTextures::init(TextureCache* cache)
{
    TextureSize size = {
        .width = 1,
        .height = 1,
        .depth = 0,
    };
    TextureInfo texture_info;
    texture_info.memory_info.size = size;
    texture_info.memory_info.origin = TextureOrigin::Void;
    texture_info.mipmaps = false;
    texture_info.memory_info.flip = false;
    texture_info.internal_format = GL_RGBA8;

    TextureSubimageInfo subimage_info;
    subimage_info.size = size;
    subimage_info.format = GL_RGBA;
    subimage_info.type = GL_UNSIGNED_BYTE;

    std::array<u8, 4> data_albedo = { 255, 255, 255, 255 };
    subimage_info.pixels = data_albedo.data();
    m_albedo = cache->create(texture_info);
    cache->get(m_albedo)->sub_image(subimage_info);

    std::array<u8, 4> data_metallic = { 255, 0, 0, 0 };
    subimage_info.pixels = data_metallic.data();
    m_metallic = cache->create(texture_info);
    cache->get(m_metallic)->sub_image(subimage_info);

    subimage_info.format = GL_RGBA;
    subimage_info.type = GL_FLOAT;
    std::array<float, 4> data_normal = { 0.5F, 0.5F, 1.0F, 0.0F };
    subimage_info.pixels = data_normal.data();
    m_normal = cache->create(texture_info);
    cache->get(m_normal)->sub_image(subimage_info);

    m_cache = cache;
    initialized = true;
}

Handle DefaultTextures::get_albedo() const
{
    util_assert(initialized == true, "not initialized");
    return m_albedo;
}

Handle DefaultTextures::get_metallic() const
{
    util_assert(initialized == true, "not initialized");
    return m_metallic;
}

Handle DefaultTextures::get_normal() const
{
    util_assert(initialized == true, "not initialized");
    return m_normal;
}

} // namespace Renderer
