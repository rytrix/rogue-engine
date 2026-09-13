#pragma once

#include "../utils/bytestream.hpp"

namespace Renderer {

struct TextureSize {
    GLint width = 0;
    GLint height = 0;
    GLint depth = 0;
};

enum struct TextureOrigin {
    Void,
    File,
    Memory,
};

struct TextureMemoryInfo {
    TextureOrigin origin = TextureOrigin::Void;
    union {
        TextureSize size {};
        const char* file_path;
    };
    char* memory = nullptr;
    GLint memory_size = 0;
    i32 expected_channels = 0;
    bool flip = true;
};

struct TextureMemory {
    u8* memory = nullptr;
    u64 memory_size = 0;
    TextureSize size {};
    i32 channels = 0;
    bool owned = true;

    void init(TextureMemoryInfo& info);
    void deinit();

    void serialize(Utils::ByteStream& stream);
    [[nodiscard]] u8* deserialize(u8* ptr);

private:
    void from_file(TextureMemoryInfo& info);
    void from_memory(TextureMemoryInfo& info);
};

struct TextureInfo {
    bool texture_loaded = false;
    union {
        TextureMemoryInfo memory_info {};
        TextureMemory texture_memory;
    };
    GLenum dimensions = GL_TEXTURE_2D;
    GLint min_filter = GL_NEAREST;
    GLint mag_filter = GL_NEAREST;
    GLint wrap_s = GL_REPEAT;
    GLint wrap_t = GL_REPEAT;
    GLint wrap_r = GL_REPEAT;
    std::array<float, 4> border_color = { 1.0F, 1.0F, 1.0F, 1.0F };
    bool mipmaps = false;
    GLint mipmap_levels = 1;
    GLenum internal_format = GL_RGBA8;
};

struct TextureSubimageInfo {
    GLint level = 0;
    TextureSize offsets {};
    TextureSize size {};
    // Specifies the format of the pixel data. The following symbolic values are accepted:
    // GL_RED, GL_RG, GL_RGB, GL_BGR, GL_RGBA, GL_BGRA, GL_DEPTH_COMPONENT, and GL_STENCIL_INDEX.
    GLenum format = GL_RGBA;
    // Specifies the data type of the pixel data. The following symbolic values are accepted:
    // GL_UNSIGNED_BYTE, GL_BYTE, GL_UNSIGNED_SHORT, GL_SHORT, GL_UNSIGNED_INT, GL_INT, GL_FLOAT,
    // GL_UNSIGNED_BYTE_3_3_2, GL_UNSIGNED_BYTE_2_3_3_REV, GL_UNSIGNED_SHORT_5_6_5, GL_UNSIGNED_SHORT_5_6_5_REV,
    // GL_UNSIGNED_SHORT_4_4_4_4, GL_UNSIGNED_SHORT_4_4_4_4_REV, GL_UNSIGNED_SHORT_5_5_5_1,
    // GL_UNSIGNED_SHORT_1_5_5_5_REV, GL_UNSIGNED_INT_8_8_8_8, GL_UNSIGNED_INT_8_8_8_8_REV,
    // GL_UNSIGNED_INT_10_10_10_2, and GL_UNSIGNED_INT_2_10_10_10_REV.
    GLenum type = GL_UNSIGNED_BYTE;
    void* pixels = nullptr;
};

class Texture : public NoCopyNoMove {
public:
    Texture() = default;
    explicit Texture(TextureInfo& info);
    void init(TextureInfo& info);
    ~Texture();

    static GLuint get_texture_unit();
    static void drop_texture_units(u32 count);
    static void reset_texture_units();

    void sub_image(TextureSubimageInfo& info);
    void bind(GLuint texture_unit);

    [[nodiscard]] GLuint64 get_bindless_texture_id();
    [[nodiscard]] bool is_bindless_texture_mapped();
    void map_bindless_texture();
    void unmap_bindless_texture();

    void set_max_anisotropy(float max_anisotropy);

    [[nodiscard]] GLuint get_id() const noexcept;

private:
    bool initialized = false;

    GLuint m_id {};
    GLenum m_dimensions {};
    bool mipmaps = false;

    bool m_bindless_texture_mapped = false;

    void generate_mipmap();
    void texture_storage(TextureSize& size, GLenum internal_format, GLint levels);
    void from_texture_memory(TextureInfo& info);
};

} // namespace Renderer
