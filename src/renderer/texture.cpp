#include "texture.hpp"

#include "extensions.hpp"

namespace {

class TextureAllocator {
public:
    explicit TextureAllocator(u32 max_textures);

    u32 next();
    void drop(u32 count = 0);
    void reset();

private:
    const u32 m_max_textures {};
    u32 m_front {};
};

TextureAllocator::TextureAllocator(u32 max_textures)
    : m_max_textures(max_textures)
{
}

u32 TextureAllocator::next()
{
    util_assert(m_front < m_max_textures,
        "attempting to call next when front is greater than max_textures");

    if (m_front >= m_max_textures) {
        reset();
    }

    LOG_TRACE(std::format("returning {}", m_front));

    return m_front++;
}

void TextureAllocator::drop(u32 count)
{
    m_front -= count;
}

void TextureAllocator::reset()
{
    m_front = 0;
}

std::unique_ptr<TextureAllocator> create_texture_allocator()
{
    int max_units {};
    glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &max_units);
    LOG_INFO(std::format("Max texture units {}", max_units));

    return std::make_unique<TextureAllocator>(static_cast<u32>(max_units));
}

std::unique_ptr<TextureAllocator> texture_unit_allocator = nullptr;

}

namespace Renderer {

void TextureMemory::init(TextureMemoryInfo& info)
{
    if (info.origin == TextureOrigin::File) {
        from_file(info);
    } else if (info.origin == TextureOrigin::Memory) {
        from_memory(info);
    } else {
        util_error("Cannot init without a texture origin");
    }
}

void TextureMemory::deinit()
{
    if (owned) {
        stbi_image_free(memory);
    }
    memory = nullptr;
}

void TextureMemory::serialize(Utils::ByteStream& stream)
{
    stream.append_bytes(&size, sizeof(size));
    stream.append_bytes(&channels, sizeof(channels));
    stream.append_bytes(&memory_size, sizeof(memory_size));
    stream.append_bytes(memory, memory_size);
}

u8* TextureMemory::deserialize(u8* ptr)
{
    *this = {};

    std::memcpy(&size, ptr, sizeof(TextureSize));
    ptr += sizeof(size);

    std::memcpy(&channels, ptr, sizeof(i32));
    ptr += sizeof(channels);

    std::memcpy(&memory_size, ptr, sizeof(u64));
    ptr += sizeof(memory_size);

    memory = ptr;
    ptr += memory_size;

    return ptr;
}

void TextureMemory::from_file(TextureMemoryInfo& info)
{
    stbi_set_flip_vertically_on_load((int)info.flip);

    memory = stbi_load(info.file_path, &size.width, &size.height, &channels, info.expected_channels);
    if (memory == nullptr) {
        util_error(std::format("failed to load texture file: \"{}\"", info.file_path));
    }
    memory_size = size.width * size.height * channels;
}

void TextureMemory::from_memory(TextureMemoryInfo& info)
{
    stbi_set_flip_vertically_on_load((int)info.flip);

    memory = stbi_load_from_memory((const stbi_uc*)info.memory, info.memory_size, &size.width, &size.height, &channels, info.expected_channels);
    if (memory == nullptr) {
        util_error("failed to load texture from memory");
    }
    memory_size = size.width * size.height * channels;
}

Texture::Texture(TextureInfo& info)
{
    init(info);
}

Texture::~Texture()
{
    // util_assert(initialized == true, "Texture::~Texture() has not been initialized");
    if (initialized) {
        glDeleteTextures(1, &m_id);
        initialized = false;
    }
}

void Texture::init(TextureInfo& info)
{
    util_assert(initialized == false, "already initialized");

    if (info.mipmaps && info.mipmap_levels != 0) {
        LOG_WARN(std::format("mipmaps set to {}, when they should be set to 0 for automatic generation", info.mipmap_levels));
    }
    if (info.mipmaps
        && info.min_filter != GL_LINEAR_MIPMAP_LINEAR
        && info.min_filter != GL_LINEAR_MIPMAP_NEAREST
        && info.min_filter != GL_NEAREST_MIPMAP_LINEAR
        && info.min_filter != GL_NEAREST_MIPMAP_NEAREST) {
        LOG_WARN("Texture mipmaps enabled but min_filter is not using mipmaps");
    }

    m_dimensions = info.dimensions;
    mipmaps = info.mipmaps;

    glCreateTextures(m_dimensions, 1, &m_id);

    glTextureParameteri(m_id, GL_TEXTURE_MIN_FILTER, info.min_filter);
    glTextureParameteri(m_id, GL_TEXTURE_MAG_FILTER, info.mag_filter);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_S, info.wrap_s);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_T, info.wrap_t);
    glTextureParameteri(m_id, GL_TEXTURE_WRAP_R, info.wrap_r);
    glTextureParameterfv(m_id, GL_TEXTURE_BORDER_COLOR, info.border_color.data());

    initialized = true;

    if (info.memory_info.origin == TextureOrigin::File || info.memory_info.origin == TextureOrigin::Memory || info.texture_loaded) {
        from_texture_memory(info);
        if (mipmaps) {
            generate_mipmap();
        }
    } else {
        texture_storage(info.memory_info.size, info.internal_format, info.mipmap_levels);
    }
}

GLuint Texture::get_texture_unit()
{
    if (texture_unit_allocator == nullptr) {
        texture_unit_allocator = create_texture_allocator();
    }
    return texture_unit_allocator->next();
}

void Texture::drop_texture_units(u32 count)
{
    if (texture_unit_allocator != nullptr) {
        texture_unit_allocator->drop(count);
    }
}

void Texture::reset_texture_units()
{
    if (texture_unit_allocator != nullptr) {
        texture_unit_allocator->reset();
    }
}

void Texture::generate_mipmap()
{
    util_assert(initialized == true, "not initialized");
    glGenerateTextureMipmap(m_id);
}

void Texture::sub_image(TextureSubimageInfo& info)
{
    util_assert(initialized == true, "not initialized");
    switch (m_dimensions) {
        case GL_TEXTURE_1D:
            glTextureSubImage1D(m_id,
                info.level,
                info.offsets.width,
                info.size.width,
                info.format,
                info.type,
                info.pixels);
            break;
        case GL_TEXTURE_2D:
            glTextureSubImage2D(m_id,
                info.level,
                info.offsets.width,
                info.offsets.height,
                info.size.width,
                info.size.height,
                info.format,
                info.type,
                info.pixels);
            break;
        case GL_TEXTURE_3D:
        case GL_TEXTURE_CUBE_MAP:
            glTextureSubImage3D(m_id,
                info.level,
                info.offsets.width,
                info.offsets.height,
                info.offsets.depth,
                info.size.width,
                info.size.height,
                info.size.depth,
                info.format,
                info.type,
                info.pixels);
            break;
        default:
            util_error(std::format("invalid texture dimensions {}\n", m_dimensions));
    }
    if (mipmaps) {
        generate_mipmap();
    }
}

void Texture::bind(GLuint texture_unit)
{
    util_assert(initialized == true, "not initialized");
    glBindTextureUnit(texture_unit, m_id);
}

[[nodiscard]] GLuint64 Texture::get_bindless_texture_id()
{
    util_assert(initialized == true, "not initialized");
    util_assert(Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture") == true, "GL_ARB_bindless_texture not supported");
    return glGetTextureHandleARB(m_id);
}

[[nodiscard]] bool Texture::is_bindless_texture_mapped()
{
    util_assert(initialized == true, "not initialized");
    util_assert(Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture") == true, "GL_ARB_bindless_texture not supported");
    return m_bindless_texture_mapped;
}

void Texture::map_bindless_texture()
{
    util_assert(initialized == true, "not initialized");
    util_assert(Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture") == true, "GL_ARB_bindless_texture not supported");
    util_assert(m_bindless_texture_mapped == false, "Attempting to map bindless texture that is already mapped");
    glMakeTextureHandleResidentARB(get_bindless_texture_id());
    m_bindless_texture_mapped = true;
}

void Texture::unmap_bindless_texture()
{
    util_assert(initialized == true, "not initialized");
    util_assert(m_bindless_texture_mapped == true, "Attempting to unmap bindless texture that is not mapped");
    util_assert(Renderer::Extensions::is_extension_supported("GL_ARB_bindless_texture") == true, "GL_ARB_bindless_texture not supported");
    glMakeTextureHandleNonResidentARB(get_bindless_texture_id());
    m_bindless_texture_mapped = false;
}

void Texture::set_max_anisotropy(float max_anisotropy)
{
    util_assert(initialized == true, "not initialized");
    auto get_hardware_max_anisotropy = []() {
        float hardware_max_anisotropy = 0.0F;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &hardware_max_anisotropy);
        return hardware_max_anisotropy;
    };
    util_assert(max_anisotropy <= get_hardware_max_anisotropy(),
        std::format("max_anisotropy \"{}\" is higher than hardware limit \"{}\"",
            max_anisotropy,
            get_hardware_max_anisotropy()));

    max_anisotropy = std::min(max_anisotropy, get_hardware_max_anisotropy());

    glTextureParameterf(m_id, GL_TEXTURE_MAX_ANISOTROPY, max_anisotropy);
}

[[nodiscard]] GLuint Texture::get_id() const noexcept
{
    util_assert(initialized == true, "not initialized");
    return m_id;
}

void Texture::texture_storage(TextureSize& size, GLenum internal_format, GLint levels)
{
    util_assert(initialized == true, "not initialized");
    if (levels == 0) {
        levels = 1 + std::floor(std::log2(std::max({ size.width, size.height, size.depth })));
    }
    switch (m_dimensions) {
        case GL_TEXTURE_1D:
            glTextureStorage1D(m_id, levels, internal_format, size.width);
            break;
        case GL_TEXTURE_2D:
            glTextureStorage2D(m_id, levels, internal_format, size.width, size.height);
            break;
        case GL_TEXTURE_3D:
            glTextureStorage3D(m_id, levels, internal_format, size.width, size.height, size.depth);
            break;
        case GL_TEXTURE_2D_ARRAY:
            glTextureStorage3D(m_id, levels, internal_format, size.width, size.height, size.depth);
            break;
        case GL_TEXTURE_CUBE_MAP:
            glTextureStorage2D(m_id, levels, internal_format, size.width, size.height);
            break;
        default:
            util_error(std::format("invalid texture dimensions {}\n", m_dimensions));
    }
}

void Texture::from_texture_memory(TextureInfo& info)
{
    if (m_dimensions != GL_TEXTURE_2D) {
        util_error("currently only 2D textures are supported from files");
    }

    TextureMemory temp_texture;
    TextureMemory& texture = temp_texture;
    if (info.texture_loaded) {
        texture = info.texture_memory;
    } else {
        texture.init(info.memory_info);
    }

    TextureSubimageInfo subimage_info {};
    subimage_info.type = GL_UNSIGNED_BYTE;
    subimage_info.size = texture.size;
    subimage_info.pixels = texture.memory;

    if (texture.channels == 3) {
        texture_storage(texture.size, GL_RGB8, info.mipmap_levels);
        subimage_info.format = GL_RGB;
    } else if (texture.channels == 4) {
        texture_storage(texture.size, GL_RGBA8, info.mipmap_levels);
        subimage_info.format = GL_RGBA;
    } else {
        util_error(std::format("invalid number of channels \"{}\"", texture.channels));
    }

    sub_image(subimage_info);
    if (!info.texture_loaded) {
        texture.deinit();
    }
}

} // namespace Renderer
